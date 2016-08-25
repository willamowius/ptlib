/*
 * winserial.cxx
 *
 * Miscellaneous implementation of classes for Win32
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
#include <ptlib/serchan.h>


#define QUEUE_SIZE 2048


///////////////////////////////////////////////////////////////////////////////
// PSerialChannel

void PSerialChannel::Construct()
{
  commsResource = INVALID_HANDLE_VALUE;

  char str[50];
  strcpy(str, "com1");
  GetProfileString("ports", str, "9600,n,8,1,x", &str[5], sizeof(str)-6);
  str[4] = ':';
  memset(&deviceControlBlock, 0, sizeof(deviceControlBlock));
  deviceControlBlock.DCBlength = sizeof(deviceControlBlock);
  BuildCommDCB(str, &deviceControlBlock);

  // These values are not set by BuildCommDCB
  deviceControlBlock.XoffChar = 19;
  deviceControlBlock.XonChar = 17;
  deviceControlBlock.XoffLim = (QUEUE_SIZE * 7)/8;  // upper limit before XOFF is sent to stop reception
  deviceControlBlock.XonLim = (QUEUE_SIZE * 3)/4;   // lower limit before XON is sent to re-enabled reception
}


PString PSerialChannel::GetName() const
{
  return portName;
}


PBoolean PSerialChannel::Read(void * buf, PINDEX len)
{
  lastReadCount = 0;

  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF, LastReadError);

  COMMTIMEOUTS cto;
  PAssertOS(GetCommTimeouts(commsResource, &cto));
  cto.ReadIntervalTimeout = 0;
  cto.ReadTotalTimeoutMultiplier = 0;
  cto.ReadTotalTimeoutConstant = 0;
  cto.ReadIntervalTimeout = MAXDWORD; // Immediate timeout
  PAssertOS(SetCommTimeouts(commsResource, &cto));

  DWORD eventMask;
  PAssertOS(GetCommMask(commsResource, &eventMask));
  if (eventMask != (EV_RXCHAR|EV_TXEMPTY))
    PAssertOS(SetCommMask(commsResource, EV_RXCHAR|EV_TXEMPTY));

  DWORD timeToGo = readTimeout.GetInterval();
  DWORD bytesToGo = len;
  char * bufferPtr = (char *)buf;

  for (;;) {
    PWin32Overlapped overlap;
    DWORD readCount = 0;
    if (!ReadFile(commsResource, bufferPtr, bytesToGo, &readCount, &overlap)) {
      if (::GetLastError() != ERROR_IO_PENDING)
        return ConvertOSError(-2, LastReadError);
      if (!::GetOverlappedResult(commsResource, &overlap, &readCount, PFalse))
        return ConvertOSError(-2, LastReadError);
    }

    bytesToGo -= readCount;
    bufferPtr += readCount;
    lastReadCount += readCount;
    if (lastReadCount >= len || timeToGo == 0)
      return lastReadCount > 0;

    if (!::WaitCommEvent(commsResource, &eventMask, &overlap)) {
      if (::GetLastError()!= ERROR_IO_PENDING)
        return ConvertOSError(-2, LastReadError);
      DWORD err = ::WaitForSingleObject(overlap.hEvent, timeToGo);
      if (err == WAIT_TIMEOUT) {
        SetErrorValues(Timeout, EAGAIN, LastReadError);
        ::CancelIo(commsResource);
        return lastReadCount > 0;
      }
      else if (err == WAIT_FAILED)
        return ConvertOSError(-2, LastReadError);
    }
  }
}


PBoolean PSerialChannel::Write(const void * buf, PINDEX len)
{
  lastWriteCount = 0;

  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF, LastWriteError);

  COMMTIMEOUTS cto;
  PAssertOS(GetCommTimeouts(commsResource, &cto));
  cto.WriteTotalTimeoutMultiplier = 0;
  if (writeTimeout == PMaxTimeInterval)
    cto.WriteTotalTimeoutConstant = 0;
  else if (writeTimeout <= PTimeInterval(0))
    cto.WriteTotalTimeoutConstant = 1;
  else
    cto.WriteTotalTimeoutConstant = writeTimeout.GetInterval();
  PAssertOS(SetCommTimeouts(commsResource, &cto));

  PWin32Overlapped overlap;
  if (WriteFile(commsResource, buf, len, (LPDWORD)&lastWriteCount, &overlap)) 
    return lastWriteCount == len;

  if (GetLastError() == ERROR_IO_PENDING)
    if (GetOverlappedResult(commsResource, &overlap, (LPDWORD)&lastWriteCount, PTrue)) {
      return lastWriteCount == len;
    }

  ConvertOSError(-2, LastWriteError);

  return PFalse;
}


PBoolean PSerialChannel::Close()
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  CloseHandle(commsResource);
  commsResource = INVALID_HANDLE_VALUE;
  os_handle = -1;
  return ConvertOSError(-2);
}


PBoolean PSerialChannel::SetCommsParam(DWORD speed, BYTE data, Parity parity,
                     BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
{
  if (speed > 0)
    deviceControlBlock.BaudRate = speed;

  if (data > 0)
    deviceControlBlock.ByteSize = data;

  switch (parity) {
    case NoParity :
      deviceControlBlock.Parity = NOPARITY;
      break;
    case OddParity :
      deviceControlBlock.Parity = ODDPARITY;
      break;
    case EvenParity :
      deviceControlBlock.Parity = EVENPARITY;
      break;
    case MarkParity :
      deviceControlBlock.Parity = MARKPARITY;
      break;
    case SpaceParity :
      deviceControlBlock.Parity = SPACEPARITY;
      break;
  }

  switch (stop) {
    case 1 :
      deviceControlBlock.StopBits = ONESTOPBIT;
      break;
    case 2 :
      deviceControlBlock.StopBits = TWOSTOPBITS;
      break;
  }

  switch (inputFlow) {
    case NoFlowControl :
      deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
      deviceControlBlock.fInX = PFalse;
      break;
    case XonXoff :
      deviceControlBlock.fRtsControl = RTS_CONTROL_DISABLE;
      deviceControlBlock.fInX = PTrue;
      break;
    case RtsCts :
      deviceControlBlock.fRtsControl = RTS_CONTROL_HANDSHAKE;
      deviceControlBlock.fInX = PFalse;
      break;
  }

  switch (outputFlow) {
    case NoFlowControl :
      deviceControlBlock.fOutxCtsFlow = PFalse;
      deviceControlBlock.fOutxDsrFlow = PFalse;
      deviceControlBlock.fOutX = PFalse;
      break;
    case XonXoff :
      deviceControlBlock.fOutxCtsFlow = PFalse;
      deviceControlBlock.fOutxDsrFlow = PFalse;
      deviceControlBlock.fOutX = PTrue;
      break;
    case RtsCts :
      deviceControlBlock.fOutxCtsFlow = PTrue;
      deviceControlBlock.fOutxDsrFlow = PFalse;
      deviceControlBlock.fOutX = PFalse;
      break;
  }

  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  return ConvertOSError(SetCommState(commsResource, &deviceControlBlock) ? 0 : -2);
}


PBoolean PSerialChannel::Open(const PString & port, DWORD speed, BYTE data,
               Parity parity, BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
{
  Close();

  portName = port;
  if (portName.Find(PDIR_SEPARATOR) == P_MAX_INDEX)
    portName = "\\\\.\\" + port;
  commsResource = CreateFile(portName,
                             GENERIC_READ|GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_FLAG_OVERLAPPED,
                             NULL);
  if (commsResource == INVALID_HANDLE_VALUE)
    return ConvertOSError(-2);

  os_handle = 0;

  SetupComm(commsResource, QUEUE_SIZE, QUEUE_SIZE);

  if (SetCommsParam(speed, data, parity, stop, inputFlow, outputFlow))
    return PTrue;

  ConvertOSError(-2);
  CloseHandle(commsResource);
  os_handle = -1;
  return PFalse;
}


PBoolean PSerialChannel::SetSpeed(DWORD speed)
{
  return SetCommsParam(speed,
                  0, DefaultParity, 0, DefaultFlowControl, DefaultFlowControl);
}


DWORD PSerialChannel::GetSpeed() const
{
  return deviceControlBlock.BaudRate;
}


PBoolean PSerialChannel::SetDataBits(BYTE data)
{
  return SetCommsParam(0,
               data, DefaultParity, 0, DefaultFlowControl, DefaultFlowControl);
}


BYTE PSerialChannel::GetDataBits() const
{
  return deviceControlBlock.ByteSize;
}


PBoolean PSerialChannel::SetParity(Parity parity)
{
  return SetCommsParam(0,0,parity,0,DefaultFlowControl,DefaultFlowControl);
}


PSerialChannel::Parity PSerialChannel::GetParity() const
{
  switch (deviceControlBlock.Parity) {
    case ODDPARITY :
      return OddParity;
    case EVENPARITY :
      return EvenParity;
    case MARKPARITY :
      return MarkParity;
    case SPACEPARITY :
      return SpaceParity;
  }
  return NoParity;
}


PBoolean PSerialChannel::SetStopBits(BYTE stop)
{
  return SetCommsParam(0,
               0, DefaultParity, stop, DefaultFlowControl, DefaultFlowControl);
}


BYTE PSerialChannel::GetStopBits() const
{
  return (BYTE)(deviceControlBlock.StopBits == ONESTOPBIT ? 1 : 2);
}


PBoolean PSerialChannel::SetInputFlowControl(FlowControl flowControl)
{
  return SetCommsParam(0,0,DefaultParity,0,flowControl,DefaultFlowControl);
}


PSerialChannel::FlowControl PSerialChannel::GetInputFlowControl() const
{
  if (deviceControlBlock.fRtsControl == RTS_CONTROL_HANDSHAKE)
    return RtsCts;
  if (deviceControlBlock.fInX != 0)
    return XonXoff;
  return NoFlowControl;
}


PBoolean PSerialChannel::SetOutputFlowControl(FlowControl flowControl)
{
  return SetCommsParam(0,0,DefaultParity,0,DefaultFlowControl,flowControl);
}


PSerialChannel::FlowControl PSerialChannel::GetOutputFlowControl() const
{
  if (deviceControlBlock.fOutxCtsFlow != 0)
    return RtsCts;
  if (deviceControlBlock.fOutX != 0)
    return XonXoff;
  return NoFlowControl;
}


void PSerialChannel::SetDTR(PBoolean state)
{
  if (IsOpen())
    PAssertOS(EscapeCommFunction(commsResource, state ? SETDTR : CLRDTR));
  else
    SetErrorValues(NotOpen, EBADF);
}


void PSerialChannel::SetRTS(PBoolean state)
{
  if (IsOpen())
    PAssertOS(EscapeCommFunction(commsResource, state ? SETRTS : CLRRTS));
  else
    SetErrorValues(NotOpen, EBADF);
}


void PSerialChannel::SetBreak(PBoolean state)
{
  if (IsOpen())
    if (state)
      PAssertOS(SetCommBreak(commsResource));
    else
      PAssertOS(ClearCommBreak(commsResource));
  else
    SetErrorValues(NotOpen, EBADF);
}


PBoolean PSerialChannel::GetCTS()
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  DWORD stat;
  PAssertOS(GetCommModemStatus(commsResource, &stat));
  return (stat&MS_CTS_ON) != 0;
}


PBoolean PSerialChannel::GetDSR()
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  DWORD stat;
  PAssertOS(GetCommModemStatus(commsResource, &stat));
  return (stat&MS_DSR_ON) != 0;
}


PBoolean PSerialChannel::GetDCD()
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  DWORD stat;
  PAssertOS(GetCommModemStatus(commsResource, &stat));
  return (stat&MS_RLSD_ON) != 0;
}


PBoolean PSerialChannel::GetRing()
{
  if (!IsOpen())
    return SetErrorValues(NotOpen, EBADF);

  DWORD stat;
  PAssertOS(GetCommModemStatus(commsResource, &stat));
  return (stat&MS_RING_ON) != 0;
}


PStringList PSerialChannel::GetPortNames()
{
  PStringList ports;
  for (char p = 1; p <= 9; p++)
    ports.AppendString(psprintf("\\\\.\\COM%u", p));
  return ports;
}
