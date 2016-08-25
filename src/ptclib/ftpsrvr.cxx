/*
 * ftpsrvr.cxx
 *
 * FTP server class.
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
#include <ptlib/sockets.h>
#include <ptclib/ftp.h>

#define new PNEW


#define READY_STRING  "PWLib FTP Server v1.0 ready"
#define GOOBYE_STRING "Goodbye"


/////////////////////////////////////////////////////////
//  FTPServer

PFTPServer::PFTPServer()
  : readyString(PIPSocket::GetHostName() & READY_STRING)
{
  Construct();
}


PFTPServer::PFTPServer(const PString & readyStr)
  : readyString(readyStr)
{
  Construct();
}


void PFTPServer::Construct()
{
  thirdPartyPort = PFalse;
  illegalPasswordCount = 0;
  state     = NotConnected;
  type      = 'A';
  structure = 'F';
  mode      = 'S';
  passiveSocket = NULL;
}


PFTPServer::~PFTPServer()
{
  delete passiveSocket;
}


PBoolean PFTPServer::OnOpen()
{
  // the default data port for a client is the same port
  PIPSocket * socket = GetSocket();
  if (socket == NULL)
    return PFalse;

  state = NeedUser;
  if (!WriteResponse(220, readyString))
    return PFalse;

  socket->GetPeerAddress(remoteHost, remotePort);
  return PTrue;
}


PString PFTPServer::GetHelloString(const PString & user) const
{
  return PString("User") & user & "logged in.";
}


PString PFTPServer::GetGoodbyeString(const PString &) const
{
  return PString(GOOBYE_STRING);
}


PString PFTPServer::GetSystemTypeString() const
{
  return PProcess::GetOSClass() + " " + PProcess::GetOSName() + " " + PProcess::GetOSVersion();
}


PBoolean PFTPServer::AuthoriseUser(const PString &, const PString &, PBoolean &)
{
  return PTrue;
}


PBoolean PFTPServer::ProcessCommand()
{
  PString args;
  PINDEX code;
  if (!ReadCommand(code, args))
    return PFalse;

  if (code == P_MAX_INDEX)
    return OnUnknown(args);

  //  handle commands that require login
  if (state == Connected || !CheckLoginRequired(code)) 
    return DispatchCommand(code, args);
  
  // otherwise enforce login
  WriteResponse(530, "Please login with USER and PASS.");
  return PTrue;
}


PBoolean PFTPServer::DispatchCommand(PINDEX code, const PString & args)
{
  switch (code) {

    // mandatory commands
    case USER:
      return OnUSER(args);
    case PASS:
      return OnPASS(args);
    case QUIT:
      return OnQUIT(args);
    case PORT:
      return OnPORT(args);
    case STRU:
      return OnSTRU(args);
    case MODE:
      return OnMODE(args);
    case NOOP:
      return OnNOOP(args);
    case TYPE:
      return OnTYPE(args);
    case RETR:
      return OnRETR(args);
    case STOR:
      return OnSTOR(args);
    case SYST:
      return OnSYST(args);
    case STATcmd:
      return OnSTAT(args);
    case ACCT:
      return OnACCT(args);
    case CWD:
      return OnCWD(args);
    case CDUP:
      return OnCDUP(args);
    case PASV:
      return OnPASV(args);
    case APPE:
      return OnAPPE(args);
    case RNFR:
      return OnRNFR(args);
    case RNTO:
      return OnRNTO(args);
    case DELE:
      return OnDELE(args);
    case RMD:
      return OnRMD(args);
    case MKD:
      return OnMKD(args);
    case PWD:
      return OnPWD(args);
    case LIST:
      return OnLIST(args);
    case NLST:
      return OnNLST(args);

    // optional commands
    case HELP:
      return OnHELP(args);
    case SITE:
      return OnSITE(args);
    case ABOR:
      return OnABOR(args);
    case SMNT:
      return OnSMNT(args);
    case REIN:
      return OnREIN(args);
    case STOU:
      return OnSTOU(args);
    case ALLO:
      return OnALLO(args);
    case REST:
      return OnREST(args);
    default:
      PAssertAlways("Registered FTP command not handled");
      return PFalse;
  }
  return PTrue;
}


PBoolean PFTPServer::CheckLoginRequired(PINDEX cmd)
{
  static const BYTE RequiresLogin[NumCommands] = {
    1, // USER
    1, // PASS
    0, // ACCT
    0, // CWD
    0, // CDUP
    0, // SMNT
    1, // QUIT
    0, // REIN
    1, // PORT
    0, // PASV
    1, // TYPE
    1, // STRU
    1, // MODE
    0, // RETR
    0, // STOR
    0, // STOU
    0, // APPE
    0, // ALLO
    0, // REST
    0, // RNFR
    0, // RNTO
    1, // ABOR
    0, // DELE
    0, // RMD
    0, // MKD
    0, // PWD
    0, // LIST
    0, // NLST
    1, // SITE
    1, // SYST
    1, // STAT
    1, // HELP
    1, // NOOP
  };
  if (cmd < NumCommands)
    return RequiresLogin[cmd] == 0;
  else
    return PTrue;
}


PBoolean PFTPServer::OnUnknown(const PCaselessString & command)
{
  WriteResponse(500, "\"" + command + "\" command unrecognised.");
  return PTrue;
}


void PFTPServer::OnError(PINDEX errorCode, PINDEX cmdNum, const char * msg)
{
  if (cmdNum < commandNames.GetSize())
    WriteResponse(errorCode, "Command \"" + commandNames[cmdNum] + "\":" + msg);
  else
    WriteResponse(errorCode, msg);
}


void PFTPServer::OnNotImplemented(PINDEX cmdNum)
{
  OnError(502, cmdNum, "not implemented");
}


void PFTPServer::OnSyntaxError(PINDEX cmdNum)
{
  OnError(501, cmdNum, "syntax error in parameters or arguments.");
}


void PFTPServer::OnCommandSuccessful(PINDEX cmdNum)
{
  if (cmdNum < commandNames.GetSize())
    WriteResponse(200, "\"" + commandNames[cmdNum] + "\" command successful.");
}


// mandatory commands that can be performed without loggin in

PBoolean PFTPServer::OnUSER(const PCaselessString & args)
{
  userName = args;
  state    = NeedPassword;
  WriteResponse(331, "Password required for " + args + ".");
  return PTrue;
}


PBoolean PFTPServer::OnPASS(const PCaselessString & args)
{
  PBoolean replied = PFalse;
  if (state != NeedPassword) 
    WriteResponse(503, "Login with USER first.");
  else if (!AuthoriseUser(userName, args, replied)) {
    if (!replied)
      WriteResponse(530, "Login incorrect.");
    if (illegalPasswordCount++ == MaxIllegalPasswords)
      return PFalse;
  } else {
    if (!replied)
      WriteResponse(230, GetHelloString(userName));
    illegalPasswordCount = 0;
    state = Connected;
  }
  return PTrue;
}


PBoolean PFTPServer::OnQUIT(const PCaselessString & userName)
{
  WriteResponse(221, GetGoodbyeString(userName));
  return PFalse;
}


PBoolean PFTPServer::OnPORT(const PCaselessString & args)
{
  PStringArray tokens = args.Tokenise(",");

  long values[6];
  PINDEX len = PMIN(args.GetSize(), 6);

  PINDEX i;
  for (i = 0; i < len; i++) {
    values[i] = tokens[i].AsInteger();
    if (values[i] < 0 || values[i] > 255)
      break;
  }
  if (i < 6) 
    OnSyntaxError(PORT);
  else {
    PIPSocket * socket = GetSocket();
    if (socket == NULL)
      OnError(590, PORT, "not available on non-TCP transport.");
    else {
      remoteHost = PIPSocket::Address((BYTE)values[0],
                              (BYTE)values[1], (BYTE)values[2], (BYTE)values[3]);
      remotePort = (WORD)(values[4]*256 + values[5]);
      if (remotePort < 1024 && remotePort != socket->GetPort()-1)
        OnError(590, PORT, "cannot access privileged port number.");
      else {
        PIPSocket::Address controlHost;
        GetSocket()->GetPeerAddress(controlHost);
        if (thirdPartyPort || remoteHost == controlHost)
          OnCommandSuccessful(PORT);
        else
          OnError(591, PORT, "three way transfer not allowed.");
      }
    }
  }
  return PTrue;
}


PBoolean PFTPServer::OnPASV(const PCaselessString &)
{
  if (passiveSocket != NULL)
    delete passiveSocket;

  passiveSocket = new PTCPSocket;
  passiveSocket->Listen();

  WORD portNo = passiveSocket->GetPort();
  PIPSocket::Address ourAddr;
  PIPSocket * socket = GetSocket();
  if (socket != NULL)
    socket->GetLocalAddress(ourAddr);
  PString str(PString::Printf,
              "Entering Passive Mode (%i,%i,%i,%i,%i,%i)",
              ourAddr.Byte1(),
              ourAddr.Byte2(),
              ourAddr.Byte3(),
              ourAddr.Byte4(),
              portNo/256, portNo%256);

  return WriteResponse(227, str);
}


PBoolean PFTPServer::OnTYPE(const PCaselessString & args)
{
  if (args.IsEmpty())
    OnSyntaxError(TYPE);
  else {
    switch (toupper(args[0])) {
      case 'A':
        type = 'A';
        break;
      case 'I':
        type = 'I';
        break;
      case 'E':
      case 'L':
        WriteResponse(504, PString("TYPE not implemented for parameter ") + args);
        return PTrue;

      default:
        OnSyntaxError(TYPE);
        return PTrue;
    }
  }
  OnCommandSuccessful(TYPE);
  return PTrue;
}


PBoolean PFTPServer::OnMODE(const PCaselessString & args)
{
  if (args.IsEmpty())
    OnSyntaxError(MODE);
  else {
    switch (toupper(args[0])) {
      case 'S':
        structure = 'S';
        break;
      case 'B':
      case 'C':
        WriteResponse(504, PString("MODE not implemented for parameter ") + args);
        return PTrue;
      default:
        OnSyntaxError(MODE);
        return PTrue;
    }
  }
  OnCommandSuccessful(MODE);
  return PTrue;
}


PBoolean PFTPServer::OnSTRU(const PCaselessString & args)
{
  if (args.IsEmpty())
    OnSyntaxError(STRU);
  else {
    switch (toupper(args[0])) {
      case 'F':
        structure = 'F';
        break;
      case 'R':
      case 'P':
        WriteResponse(504, PString("STRU not implemented for parameter ") + args);
        return PTrue;
      default:
        OnSyntaxError(STRU);
        return PTrue;
    }
  }
  OnCommandSuccessful(STRU);
  return PTrue;
}


PBoolean PFTPServer::OnNOOP(const PCaselessString &)
{
  OnCommandSuccessful(NOOP);
  return PTrue;
}


// mandatory commands that cannot be performed without logging in

PBoolean PFTPServer::OnRETR(const PCaselessString &)
{
  OnNotImplemented(RETR);
  return PTrue;
}


PBoolean PFTPServer::OnSTOR(const PCaselessString &)
{
  OnNotImplemented(STOR);
  return PTrue;
}


PBoolean PFTPServer::OnACCT(const PCaselessString &)
{
  WriteResponse(532, "Need account for storing files");
  return PTrue;
}


PBoolean PFTPServer::OnCWD(const PCaselessString &)
{
  OnNotImplemented(CWD);
  return PTrue;
}


PBoolean PFTPServer::OnCDUP(const PCaselessString &)
{
  OnNotImplemented(CDUP);
  return PTrue;
}


PBoolean PFTPServer::OnSMNT(const PCaselessString &)
{
  OnNotImplemented(SMNT);
  return PTrue;
}


PBoolean PFTPServer::OnREIN(const PCaselessString &)
{
  OnNotImplemented(REIN);
  return PTrue;
}


PBoolean PFTPServer::OnSTOU(const PCaselessString &)
{
  OnNotImplemented(STOU);
  return PTrue;
}


PBoolean PFTPServer::OnAPPE(const PCaselessString &)
{
  OnNotImplemented(APPE);
  return PTrue;
}


PBoolean PFTPServer::OnALLO(const PCaselessString &)
{
  OnNotImplemented(ALLO);
  return PTrue;
}


PBoolean PFTPServer::OnREST(const PCaselessString &)
{
  OnNotImplemented(REST);
  return PTrue;
}


PBoolean PFTPServer::OnRNFR(const PCaselessString &)
{
  OnNotImplemented(RNFR);
  return PTrue;
}


PBoolean PFTPServer::OnRNTO(const PCaselessString &)
{
  OnNotImplemented(RNTO);
  return PTrue;
}


PBoolean PFTPServer::OnABOR(const PCaselessString &)
{
  OnNotImplemented(ABOR);
  return PTrue;
}


PBoolean PFTPServer::OnDELE(const PCaselessString &)
{
  OnNotImplemented(DELE);
  return PTrue;
}


PBoolean PFTPServer::OnRMD(const PCaselessString &)
{
  OnNotImplemented(RMD);
  return PTrue;
}


PBoolean PFTPServer::OnMKD(const PCaselessString &)
{
  OnNotImplemented(MKD);
  return PTrue;
}


PBoolean PFTPServer::OnPWD(const PCaselessString &)
{
  OnNotImplemented(PWD);
  return PTrue;
}


PBoolean PFTPServer::OnLIST(const PCaselessString &)
{
  OnNotImplemented(LIST);
  return PTrue;
}


PBoolean PFTPServer::OnNLST(const PCaselessString &)
{
  OnNotImplemented(NLST);
  return PTrue;
}


PBoolean PFTPServer::OnSITE(const PCaselessString &)
{
  OnNotImplemented(SITE);
  return PTrue;
}


PBoolean PFTPServer::OnSYST(const PCaselessString &)
{
  WriteResponse(215, GetSystemTypeString());
  return PTrue;
}


PBoolean PFTPServer::OnSTAT(const PCaselessString &)
{
  OnNotImplemented(STATcmd);
  return PTrue;
}


PBoolean PFTPServer::OnHELP(const PCaselessString &)
{
  OnNotImplemented(HELP);
  return PTrue;
}


void PFTPServer::SendToClient(const PFilePath & filename)
{
  if (!PFile::Exists(filename)) 
    WriteResponse(450, filename + ": file not found");
  else {
    PTCPSocket * dataSocket;
    if (passiveSocket != NULL) {
      dataSocket = new PTCPSocket(*passiveSocket);
      delete passiveSocket;
      passiveSocket = NULL;
    } else
      dataSocket = new PTCPSocket(remoteHost, remotePort);
    if (!dataSocket->IsOpen())
      WriteResponse(425, "Cannot open data connection");
    else {
      if (type == 'A') {
        PTextFile file(filename, PFile::ReadOnly);
        if (!file.IsOpen())
          WriteResponse(450, filename + ": cannot open file");
        else {
          PString fileSize(PString::Unsigned, file.GetLength());
          WriteResponse(150, PString("Opening ASCII data connection for " + filename.GetFileName() + "(" + fileSize + " bytes)"));
          PString line;
          PBoolean ok = PTrue;
          while (ok && file.ReadLine(line)) {
            if (!dataSocket->Write((const char *)line, line.GetLength())) {
              WriteResponse(426, "Connection closed - transfer aborted");
              ok = PFalse;
            }
          }
          file.Close();
        }
      } else {
        PFile file(filename, PFile::ReadOnly);
        if (!file.IsOpen())
          WriteResponse(450, filename + ": cannot open file");
        else {
          PString fileSize(PString::Unsigned, file.GetLength());
          WriteResponse(150, PString("Opening BINARY data connection for " + filename.GetFileName() + "(" + fileSize + " bytes)"));
          BYTE buffer[2048];
          PBoolean ok = PTrue;
          while (ok && file.Read(buffer, 2048)) {
            if (!dataSocket->Write(buffer, file.GetLastReadCount())) {
              WriteResponse(426, "Connection closed - transfer aborted");
              ok = PFalse;
            }
          }
          file.Close();
        }
      }
      delete dataSocket;
      WriteResponse(226, "Transfer complete");
    }
  }
}


// End of File ///////////////////////////////////////////////////////////////
