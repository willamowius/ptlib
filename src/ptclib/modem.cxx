/*
 * modem.cxx
 *
 * Asynchronous Serial I/O channel class.
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
#pragma implementation "modem.h"
#endif

#include <ptlib.h>
#include <ptclib/modem.h>

#include <ctype.h>


///////////////////////////////////////////////////////////////////////////////
// PModem

PModem::PModem()
{
  status = Unopened;
}


PModem::PModem(const PString & port, DWORD speed, BYTE data,
      Parity parity, BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
  : PSerialChannel(port, speed, data, parity, stop, inputFlow, outputFlow)
{
  status = IsOpen() ? Uninitialised : Unopened;
}


#if P_CONFIG_FILE
PModem::PModem(PConfig & cfg)
{
  status = Open(cfg) ? Uninitialised : Unopened;
}
#endif // P_CONFIG_FILE


void PModem::SetInitString(const PString & str)
{
  initCmd = str;
}

PString PModem::GetInitString() const
{
  return initCmd;
}

void PModem::SetDeinitString(const PString & str)
{
  deinitCmd = str;
}

PString PModem::GetDeinitString() const
{
  return deinitCmd;
}

void PModem::SetPreDialString(const PString & str)
{
  preDialCmd = str;
}

PString PModem::GetPreDialString() const
{
  return preDialCmd;
}

void PModem::SetPostDialString(const PString & str)
{
  postDialCmd = str;
}

PString PModem::GetPostDialString() const
{
  return postDialCmd;
}

void PModem::SetBusyString(const PString & str)
{
  busyReply = str;
}

PString PModem::GetBusyString() const
{
  return busyReply;
}

void PModem::SetNoCarrierString(const PString & str)
{
  noCarrierReply = str;
}

PString PModem::GetNoCarrierString() const
{ 
  return noCarrierReply;
}

void PModem::SetConnectString(const PString & str)
{
  connectReply = str;
}

PString PModem::GetConnectString() const
{
  return connectReply;
}

void PModem::SetHangUpString(const PString & str)
{
  hangUpCmd = str;
}

PString PModem::GetHangUpString() const
{
  return hangUpCmd;
}

PModem::Status PModem::GetStatus() const
{
  return status;
}


PBoolean PModem::Close()
{
  status = Unopened;
  return PSerialChannel::Close();
}


PBoolean PModem::Open(const PString & port, DWORD speed, BYTE data,
      Parity parity, BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
{
  if (!PSerialChannel::Open(port,
                            speed, data, parity, stop, inputFlow, outputFlow))
    return PFalse;

  status = Uninitialised;
  return PTrue;
}


#if P_CONFIG_FILE

static const char ModemInit[] = "ModemInit";
static const char ModemDeinit[] = "ModemDeinit";
static const char ModemPreDial[] = "ModemPreDial";
static const char ModemPostDial[] = "ModemPostDial";
static const char ModemBusy[] = "ModemBusy";
static const char ModemNoCarrier[] = "ModemNoCarrier";
static const char ModemConnect[] = "ModemConnect";
static const char ModemHangUp[] = "ModemHangUp";

PBoolean PModem::Open(PConfig & cfg)
{
  initCmd = cfg.GetString(ModemInit, "ATZ\\r\\w2sOK\\w100m");
  deinitCmd = cfg.GetString(ModemDeinit, "\\d2s+++\\d2sATH0\\r");
  preDialCmd = cfg.GetString(ModemPreDial, "ATDT");
  postDialCmd = cfg.GetString(ModemPostDial, "\\r");
  busyReply = cfg.GetString(ModemBusy, "BUSY");
  noCarrierReply = cfg.GetString(ModemNoCarrier, "NO CARRIER");
  connectReply = cfg.GetString(ModemConnect, "CONNECT");
  hangUpCmd = cfg.GetString(ModemHangUp, "\\d2s+++\\d2sATH0\\r");

  if (!PSerialChannel::Open(cfg))
    return PFalse;

  status = Uninitialised;
  return PTrue;
}


void PModem::SaveSettings(PConfig & cfg)
{
  PSerialChannel::SaveSettings(cfg);
  cfg.SetString(ModemInit, initCmd);
  cfg.SetString(ModemDeinit, deinitCmd);
  cfg.SetString(ModemPreDial, preDialCmd);
  cfg.SetString(ModemPostDial, postDialCmd);
  cfg.SetString(ModemBusy, busyReply);
  cfg.SetString(ModemNoCarrier, noCarrierReply);
  cfg.SetString(ModemConnect, connectReply);
  cfg.SetString(ModemHangUp, hangUpCmd);
}
#endif // P_CONFIG_FILE


PBoolean PModem::CanInitialise() const
{
  switch (status) {
    case Unopened :
    case Initialising :
    case Dialling :
    case AwaitingResponse :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return PFalse;

    default :
      return PTrue;
  }
}


PBoolean PModem::Initialise()
{
  if (CanInitialise()) {
    status = Initialising;
    if (SendCommandString(initCmd)) {
      status = Initialised;
      return PTrue;
    }
    status = InitialiseFailed;
  }
  return PFalse;
}


PBoolean PModem::CanDeinitialise() const
{
  switch (status) {
    case Unopened :
    case Initialising :
    case Dialling :
    case AwaitingResponse :
    case Connected :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return PFalse;

    default :
      return PTrue;
  }
}


PBoolean PModem::Deinitialise()
{
  if (CanDeinitialise()) {
    status = Deinitialising;
    if (SendCommandString(deinitCmd)) {
      status = Uninitialised;
      return PTrue;
    }
    status = DeinitialiseFailed;
  }
  return PFalse;
}


PBoolean PModem::CanDial() const
{
  switch (status) {
    case Unopened :
    case Uninitialised :
    case Initialising :
    case InitialiseFailed :
    case Dialling :
    case AwaitingResponse :
    case Connected :
    case HangingUp :
    case Deinitialising :
    case DeinitialiseFailed :
    case SendingUserCommand :
      return PFalse;

    default :
      return PTrue;
  }
}


PBoolean PModem::Dial(const PString & number)
{
  if (!CanDial())
    return PFalse;

  status = Dialling;
  if (!SendCommandString(preDialCmd + "\\s" + number + postDialCmd)) {
    status = DialFailed;
    return PFalse;
  }

  status = AwaitingResponse;

  PTimer timeout = 120000;
  PINDEX connectPosition = 0;
  PINDEX busyPosition = 0;
  PINDEX noCarrierPosition = 0;

  for (;;) {
    int nextChar;
    if ((nextChar = ReadCharWithTimeout(timeout)) < 0)
      return PFalse;

    if (ReceiveCommandString(nextChar, connectReply, connectPosition, 0))
      break;

    if (ReceiveCommandString(nextChar, busyReply, busyPosition, 0)) {
      status = LineBusy;
      return PFalse;
    }

    if (ReceiveCommandString(nextChar, noCarrierReply, noCarrierPosition, 0)) {
      status = NoCarrier;
      return PFalse;
    }
  }

  status = Connected;
  return PTrue;
}


PBoolean PModem::CanHangUp() const
{
  switch (status) {
    case Unopened :
    case Uninitialised :
    case Initialising :
    case InitialiseFailed :
    case Dialling :
    case AwaitingResponse :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return PFalse;

    default :
      return PTrue;
  }
}


PBoolean PModem::HangUp()
{
  if (CanHangUp()) {
    status = HangingUp;
    if (SendCommandString(hangUpCmd)) {
      status = Initialised;
      return PTrue;
    }
    status = HangUpFailed;
  }
  return PFalse;
}


PBoolean PModem::CanSendUser() const
{
  switch (status) {
    case Unopened :
    case Uninitialised :
    case Initialising :
    case InitialiseFailed :
    case Dialling :
    case AwaitingResponse :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return PFalse;

    default :
      return PTrue;
  }
}


PBoolean PModem::SendUser(const PString & str)
{
  if (CanSendUser()) {
    Status oldStatus = status;
    status = SendingUserCommand;
    if (SendCommandString(str)) {
      status = oldStatus;
      return PTrue;
    }
    status = oldStatus;
  }
  return PFalse;
}


void PModem::Abort()
{
  switch (status) {
    case Initialising :
      status = InitialiseFailed;
      break;
    case Dialling :
    case AwaitingResponse :
      status = DialFailed;
      break;
    case HangingUp :
      status = HangUpFailed;
      break;
    case Deinitialising :
      status = DeinitialiseFailed;
      break;
    default :
      break;
  }
}


PBoolean PModem::CanRead() const
{
  switch (status) {
    case Unopened :
    case Initialising :
    case Dialling :
    case AwaitingResponse :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return PFalse;

    default :
      return PTrue;
  }
}


// End Of File ///////////////////////////////////////////////////////////////
