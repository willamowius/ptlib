/*
 * msdos.cxx
 *
 * General class implementation for MS-DOS
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

#include "ptlib.h"

#include <bios.h>
#include <fcntl.h>


///////////////////////////////////////////////////////////////////////////////
// PTime

PString PTime::GetTimeSeparator()
{
  return "";
}


PBoolean PTime::GetTimeAMPM()
{
  return PFalse;
}


PString PTime::GetTimeAM()
{
  return "am";
}


PString PTime::GetTimePM()
{
  return "pm";
}


PString PTime::GetDayName(Weekdays dayOfWeek, NameType type)
{
  static const char * const weekdays[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
  };
  static const char * const abbrev_weekdays[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  return (type != FullName ? abbrev_weekdays : weekdays)[dayOfWeek];
}


PString PTime::GetDateSeparator()
{
  return "-";
}


PString PTime::GetMonthName(Months month, NameType type)
{
  static const char * const months[] = { "",
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };
  static const char * const abbrev_months[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  return (type != FullName ? abbrev_months : months)[month];
}


PTime::DateOrder PTime::GetDateOrder()
{
  return DayMonthYear;
}


long PTime::GetTimeZone() const
{
  return 0;
}


PString PTime::GetTimeZoneString(TimeZoneType type) const
{
  return "";
}



///////////////////////////////////////////////////////////////////////////////
// PSerialChannel

void PSerialChannel::Construct()
{
  biosParm = 0xe3; // 9600 baud, no parity, 1 stop bit, 8 data bits
}


PString PSerialChannel::GetName() const
{
  if (IsOpen())
    return psprintf("COM%i", os_handle+1);

  return PString();
}


PBoolean PSerialChannel::Read(void * buf, PINDEX len)
{
  char * b = (char *)buf;
  while (len > 0) {
    int c = ReadChar();
    if (c >= 0) {
      *b++ = (char)c;
      len--;
    }
  }
  return len == 0;
}


PBoolean PSerialChannel::Write(const void * buf, PINDEX len)
{
  const char * b = (const char *)buf;
  while (len-- > 0) {
    if (!WriteChar(*b++))
      return PFalse;
  }
  return PTrue;
}


PBoolean PSerialChannel::Close()
{
  if (!IsOpen())
    return PFalse;

  os_handle = -1;
  return PTrue;
}


PBoolean PSerialChannel::SetCommsParam(DWORD speed, BYTE data, Parity parity,
                     BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
{
  switch (speed) {
    case 0 :
      break;
    case 110 :
      biosParm &= 0x1f;
      break;
    case 150 :
      biosParm &= 0x1f;
      biosParm |= 0x20;
      break;
    case 300 :
      biosParm &= 0x1f;
      biosParm |= 0x40;
      break;
    case 600 :
      biosParm &= 0x1f;
      biosParm |= 0x60;
      break;
    case 1200 :
      biosParm &= 0x1f;
      biosParm |= 0x80;
      break;
    case 2400 :
      biosParm &= 0x1f;
      biosParm |= 0xa0;
      break;
    case 4800 :
      biosParm &= 0x1f;
      biosParm |= 0xc0;
      break;
    case 9600 :
      biosParm &= 0x1f;
      biosParm |= 0xe0;
      break;
    default :
      return PFalse;
  }

  switch (data) {
    case 0 :
      break;
    case 5 :
      biosParm &= 0xfc;
      break;
    case 6 :
      biosParm &= 0xfc;
      biosParm |= 1;
      break;
    case 7 :
      biosParm &= 0xfc;
      biosParm |= 2;
      break;
    case 8 :
      biosParm &= 0xfc;
      biosParm |= 3;
      break;
    default :
      return PFalse;
  }

  switch (parity) {
    case DefaultParity :
      break;
    case NoParity :
      biosParm &= 0xe7;
      break;
    case OddParity :
      biosParm &= 0xe7;
      biosParm |= 8;
      break;
    case EvenParity :
      biosParm &= 0xe7;
      biosParm |= 0x10;
      break;
    default :
      return PFalse;
  }

  switch (stop) {
    case 0 :
      break;
    case 1 :
      biosParm &= ~4;
      break;
    case 2 :
      biosParm |= 4;
      break;
    default :
      return PFalse;
  }

  if (outputFlow != DefaultFlowControl || inputFlow != DefaultFlowControl)
    return PFalse;

  _bios_serialcom(_COM_INIT, os_handle, biosParm);
  return PTrue;
}

PBoolean PSerialChannel::Open(const PString & port, DWORD speed, BYTE data,
       Parity parity, BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
{
  Close();

  os_handle = -1;
  if (PCaselessString("COM") != port.Left(3) &&
                                              port[3] >= '1' && port[3] <= '4')
    return PFalse;
  os_handle = port[3] - '1';
  return SetCommsParam(speed, data, parity, stop, inputFlow, outputFlow);
}


PBoolean PSerialChannel::SetSpeed(DWORD speed)
{
  return SetCommsParam(speed,
                 0, DefaultParity, 0, DefaultFlowControl, DefaultFlowControl);
}


DWORD PSerialChannel::GetSpeed() const
{
  static int speed[8] = { 110, 150, 300, 600, 1200, 2400, 4800, 9600 };
  return speed[biosParm>>5];
}


PBoolean PSerialChannel::SetDataBits(BYTE data)
{
  return SetCommsParam(0,
              data, DefaultParity, 0, DefaultFlowControl, DefaultFlowControl);
}


BYTE PSerialChannel::GetDataBits() const
{
  return (BYTE)((biosParm&3)+5);
}


PBoolean PSerialChannel::SetParity(Parity parity)
{
  return SetCommsParam(0,0, parity, 0, DefaultFlowControl, DefaultFlowControl);
}


PSerialChannel::Parity PSerialChannel::GetParity() const
{
  return (biosParm&8) == 0 ? NoParity :
                                (biosParm&0x10) == 0 ? OddParity : EvenParity;
}


PBoolean PSerialChannel::SetStopBits(BYTE stop)
{
  return SetCommsParam(0,
               0, DefaultParity, stop, DefaultFlowControl, DefaultFlowControl);
}


BYTE PSerialChannel::GetStopBits() const
{
  return (BYTE)(((biosParm&4)>>3)+1);
}


PBoolean PSerialChannel::SetInputFlowControl(FlowControl flowControl)
{
  return SetCommsParam(0,0, DefaultParity, 0, flowControl, DefaultFlowControl);
}


PSerialChannel::FlowControl PSerialChannel::GetInputFlowControl() const
{
  return RtsCts;
}


PBoolean PSerialChannel::SetOutputFlowControl(FlowControl flowControl)
{
  return SetCommsParam(0,0, DefaultParity, 0, DefaultFlowControl, flowControl);
}


PSerialChannel::FlowControl PSerialChannel::GetOutputFlowControl() const
{
  return RtsCts;
}


void PSerialChannel::SetDTR(PBoolean state)
{
  if (!IsOpen())
    return;

}


void PSerialChannel::SetRTS(PBoolean state)
{
  if (!IsOpen())
    return;

}


void PSerialChannel::SetBreak(PBoolean state)
{
  if (!IsOpen())
    return;

  int s = state;
}


PBoolean PSerialChannel::GetCTS()
{
  if (!IsOpen())
    return PFalse;

  return (_bios_serialcom(_COM_STATUS, os_handle, 0)&0x8010) == 0x10;
}


PBoolean PSerialChannel::GetDSR()
{
  if (!IsOpen())
    return PFalse;

  return (_bios_serialcom(_COM_STATUS, os_handle, 0)&0x8020) == 0x20;
}


PBoolean PSerialChannel::GetDCD()
{
  if (!IsOpen())
    return PFalse;

  return (_bios_serialcom(_COM_STATUS, os_handle, 0)&0x8080) == 0x80;
}


PBoolean PSerialChannel::GetRing()
{
  if (!IsOpen())
    return PFalse;

  return (_bios_serialcom(_COM_STATUS, os_handle, 0)&0x8040) == 0x40;
}


PStringList PSerialChannel::GetPortNames()
{
  static char buf[] = "COM ";
  PStringList ports;
  for (char p = '1'; p <= '4'; p++) {
    if (*(WORD *)(0x00400000+p-'1') != 0) {
      buf[3] = p;
      ports.Append(new PString(buf));
    }
  }
  return ports;
}


///////////////////////////////////////////////////////////////////////////////
// PPipeChannel

PBoolean PPipeChannel::Execute()
{
  if (hasRun)
    return PFalse;

  flush();
  if (os_handle >= 0) {
    _close(os_handle);
    os_handle = -1;
  }

  if (!ConvertOSError(system(subProgName)))
    return PFalse;

  if (!fromChild.IsEmpty()) {
    os_handle = _open(fromChild, _O_RDONLY);
    if (!ConvertOSError(os_handle))
      return PFalse;
  }

  return PTrue;
}


///////////////////////////////////////////////////////////////////////////////
// Configuration files

void PConfig::Construct(Source src)
{
  switch (src) {
    case Application :
      PFilePath appFile = PProcess::Current()->GetFile();
      location = appFile.GetVolume() +
                              appFile.GetPath() + appFile.GetTitle() + ".INI";
  }
}


void PConfig::Construct(const PFilePath & file)
{
  location = file;
}


PStringList PConfig::GetSections()
{
  PStringList sections;

  if (!location.IsEmpty()) {
    PAssertAlways(PUnimplementedFunction);
  }

  return sections;
}


PStringList PConfig::GetKeys(const PString &) const
{
  PStringList keys;

  if (location.IsEmpty()) {
    char ** ptr = _environ;
    while (*ptr != NULL) {
      PString buf = *ptr++;
      keys.AppendString(buf.Left(buf.Find('=')));
    }
  }
  else {
    PAssertAlways(PUnimplementedFunction);
  }

  return keys;
}


void PConfig::DeleteSection(const PString &)
{
  if (location.IsEmpty())
    return;

  PAssertAlways(PUnimplementedFunction);
}


void PConfig::DeleteKey(const PString &, const PString & key)
{
  if (location.IsEmpty()) {
    PAssert(key.Find('=') == P_MAX_INDEX, PInvalidParameter);
    _putenv(key + "=");
  }
  else
    PAssertAlways(PUnimplementedFunction);
}


PString PConfig::GetString(const PString &,
                                          const PString & key, const PString & dflt)
{
  PString str;

  if (location.IsEmpty()) {
    PAssert(key.Find('=') == P_MAX_INDEX, PInvalidParameter);
    char * env = getenv(key);
    if (env != NULL)
      str = env;
    else
      str = dflt;
  }
  else {
    PAssertAlways(PUnimplementedFunction);
  }
  return str;
}


void PConfig::SetString(const PString &, const PString & key, const PString & value)
{
  if (location.IsEmpty()) {
    PAssert(key.Find('=') == P_MAX_INDEX, PInvalidParameter);
    _putenv(key + "=" + value);
  }
  else
    PAssertAlways(PUnimplementedFunction);
}



///////////////////////////////////////////////////////////////////////////////
// Threads

void PThread::SwitchContext(PThread * from)
{
  if (setjmp(from->context) != 0) // Are being reactivated from previous yield
    return;

  if (status == Starting) {
    if (setjmp(context) != 0) {
      status = Running;
      Main();
      Terminate(); // Never returns from here
    }
    context[7] = (int)stackTop-16;  // Change the stack pointer in jmp_buf
  }

  longjmp(context, PTrue);
  PAssertAlways("longjmp failed"); // Should never get here
}



///////////////////////////////////////////////////////////////////////////////
// PDynaLink

PDynaLink::PDynaLink()
{
  PAssertAlways(PUnimplementedFunction);
}


PDynaLink::PDynaLink(const PString &)
{
  PAssertAlways(PUnimplementedFunction);
}


PDynaLink::~PDynaLink()
{
}


PBoolean PDynaLink::Open(const PString & name)
{
  PAssertAlways(PUnimplementedFunction);
  return PFalse;
}


void PDynaLink::Close()
{
}


PBoolean PDynaLink::IsLoaded() const
{
  return PFalse;
}


PBoolean PDynaLink::GetFunction(PINDEX index, Function & func)
{
  return PFalse;
}


PBoolean PDynaLink::GetFunction(const PString & name, Function & func)
{
  return PFalse;
}



// End Of File ///////////////////////////////////////////////////////////////
