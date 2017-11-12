/*
 * osutil.inl
 *
 * Operating System Classes Inline Function Definitions
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
 * $Id$
 */

#include "ptbuildopts.h"

///////////////////////////////////////////////////////////////////////////////
// PTimeInterval

PINLINE PTimeInterval::PTimeInterval(PInt64 millisecs)
  : m_milliseconds(millisecs) { }


PINLINE PObject * PTimeInterval::Clone() const
  { return PNEW PTimeInterval(GetMilliSeconds()); }

PINLINE long PTimeInterval::GetSeconds() const
  { return (long)(GetMilliSeconds()/1000); }

PINLINE long PTimeInterval::GetMinutes() const
  { return (long)(GetMilliSeconds()/60000); }

PINLINE int PTimeInterval::GetHours() const
  { return (int)(GetMilliSeconds()/3600000); }

PINLINE int PTimeInterval::GetDays() const
  { return (int)(GetMilliSeconds()/86400000); }


PINLINE PTimeInterval PTimeInterval::operator-() const
  { return PTimeInterval(-GetMilliSeconds()); }

PINLINE PTimeInterval PTimeInterval::operator+(const PTimeInterval & t) const
  { return PTimeInterval(GetMilliSeconds() + t.GetMilliSeconds()); }

PINLINE PTimeInterval & PTimeInterval::operator+=(const PTimeInterval & t)
  { SetMilliSeconds(GetMilliSeconds() + t.GetMilliSeconds()); return *this; }

PINLINE PTimeInterval PTimeInterval::operator-(const PTimeInterval & t) const
  { return PTimeInterval(GetMilliSeconds() - t.GetMilliSeconds()); }

PINLINE PTimeInterval & PTimeInterval::operator-=(const PTimeInterval & t)
  { SetMilliSeconds(GetMilliSeconds() - t.GetMilliSeconds()); return *this; }

PINLINE PTimeInterval PTimeInterval::operator*(int f) const
  { return PTimeInterval(GetMilliSeconds() * f); }

PINLINE PTimeInterval & PTimeInterval::operator*=(int f)
  { SetMilliSeconds(GetMilliSeconds() * f); return *this; }

PINLINE int PTimeInterval::operator/(const PTimeInterval & t) const
  { return (int)(GetMilliSeconds() / t.GetMilliSeconds()); }

PINLINE PTimeInterval PTimeInterval::operator/(int f) const
  { return PTimeInterval(GetMilliSeconds() / f); }

PINLINE PTimeInterval & PTimeInterval::operator/=(int f)
  { SetMilliSeconds(GetMilliSeconds() / f); return *this; }


PINLINE bool PTimeInterval::operator==(const PTimeInterval & t) const
  { return GetMilliSeconds() == t.GetMilliSeconds(); }

PINLINE bool PTimeInterval::operator!=(const PTimeInterval & t) const
  { return GetMilliSeconds() != t.GetMilliSeconds(); }

PINLINE bool PTimeInterval::operator> (const PTimeInterval & t) const
  { return GetMilliSeconds() > t.GetMilliSeconds(); }

PINLINE bool PTimeInterval::operator>=(const PTimeInterval & t) const
  { return GetMilliSeconds() >= t.GetMilliSeconds(); }

PINLINE bool PTimeInterval::operator< (const PTimeInterval & t) const
  { return GetMilliSeconds() < t.GetMilliSeconds(); }

PINLINE bool PTimeInterval::operator<=(const PTimeInterval & t) const
  { return GetMilliSeconds() <= t.GetMilliSeconds(); }

PINLINE bool PTimeInterval::operator==(long msecs) const
  { return (long)GetMilliSeconds() == msecs; }

PINLINE bool PTimeInterval::operator!=(long msecs) const
  { return (long)GetMilliSeconds() != msecs; }

PINLINE bool PTimeInterval::operator> (long msecs) const
  { return (long)GetMilliSeconds() > msecs; }

PINLINE bool PTimeInterval::operator>=(long msecs) const
  { return (long)GetMilliSeconds() >= msecs; }

PINLINE bool PTimeInterval::operator< (long msecs) const
  { return (long)GetMilliSeconds() < msecs; }

PINLINE bool PTimeInterval::operator<=(long msecs) const
  { return (long)GetMilliSeconds() <= msecs; }


///////////////////////////////////////////////////////////////////////////////
// PTime

PINLINE PObject * PTime::Clone() const
  { return PNEW PTime(theTime, microseconds); }

PINLINE void PTime::PrintOn(ostream & strm) const
  { strm << AsString(); }

PINLINE PBoolean PTime::IsValid() const
  { return theTime > 46800; }

PINLINE PInt64 PTime::GetTimestamp() const
  { return theTime*(PInt64)1000000 + microseconds; }

PINLINE time_t PTime::GetTimeInSeconds() const
  { return theTime; }

PINLINE long PTime::GetMicrosecond() const
  { return microseconds; }

PINLINE int PTime::GetSecond() const
  { struct tm ts; return os_localtime(&theTime, &ts)->tm_sec; }

PINLINE int PTime::GetMinute() const
  { struct tm ts; return os_localtime(&theTime, &ts)->tm_min; }

PINLINE int PTime::GetHour() const
  { struct tm ts; return os_localtime(&theTime, &ts)->tm_hour; }

PINLINE int PTime::GetDay() const
  { struct tm ts; return os_localtime(&theTime, &ts)->tm_mday; }

PINLINE PTime::Months PTime::GetMonth() const
  { struct tm ts; return (Months)(os_localtime(&theTime, &ts)->tm_mon+January); }

PINLINE int PTime::GetYear() const
  { struct tm ts; return os_localtime(&theTime, &ts)->tm_year+1900; }

PINLINE PTime::Weekdays PTime::GetDayOfWeek() const
  { struct tm ts; return (Weekdays)os_localtime(&theTime, &ts)->tm_wday; }

PINLINE int PTime::GetDayOfYear() const
  { struct tm ts; return os_localtime(&theTime, &ts)->tm_yday; }

PINLINE PBoolean PTime::IsPast() const
  { return GetTimeInSeconds() < PTime().GetTimeInSeconds(); }

PINLINE PBoolean PTime::IsFuture() const
  { return GetTimeInSeconds() > PTime().GetTimeInSeconds(); }


PINLINE PString PTime::AsString(const PString & format, int zone) const
  { return AsString((const char *)format, zone); }

PINLINE int PTime::GetTimeZone() 
  { return GetTimeZone(IsDaylightSavings() ? DaylightSavings : StandardTime); }


///////////////////////////////////////////////////////////////////////////////
// PSimpleTimer

PINLINE void PSimpleTimer::Stop()
  { SetInterval(0); }

PINLINE PTimeInterval PSimpleTimer::GetElapsed() const
  { return PTimer::Tick() - m_startTick; }

PINLINE PTimeInterval PSimpleTimer::GetRemaining() const
  { return *this - (PTimer::Tick() - m_startTick); }

PINLINE bool PSimpleTimer::IsRunning() const
  { return (PTimer::Tick() - m_startTick) < *this; }

PINLINE bool PSimpleTimer::HasExpired() const
  { return (PTimer::Tick() - m_startTick) >= *this; }

PINLINE PSimpleTimer::operator bool() const
  { return HasExpired(); }


///////////////////////////////////////////////////////////////////////////////
// PTimer

PINLINE PBoolean PTimer::IsRunning() const
  { return m_state == Running; }

PINLINE PBoolean PTimer::IsPaused() const
  { return m_state == Paused; }

PINLINE const PTimeInterval & PTimer::GetResetTime() const
  { return m_resetTime; }

PINLINE const PNotifier & PTimer::GetNotifier() const
  { return m_callback; }

PINLINE void PTimer::SetNotifier(const PNotifier & func)
  { m_callback = func; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PChannelStreamBuffer::PChannelStreamBuffer(const PChannelStreamBuffer & sbuf)
  : channel(sbuf.channel) { }

PINLINE PChannelStreamBuffer &
          PChannelStreamBuffer::operator=(const PChannelStreamBuffer & sbuf)
  { channel = sbuf.channel; return *this; }

PINLINE PChannel::PChannel(const PChannel &) : iostream(cout.rdbuf())
  { PAssertAlways("Cannot copy channels"); }

PINLINE PChannel & PChannel::operator=(const PChannel &)
  { PAssertAlways("Cannot assign channels"); return *this; }

PINLINE void PChannel::SetReadTimeout(const PTimeInterval & time)
  { readTimeout = time; }

PINLINE PTimeInterval PChannel::GetReadTimeout() const
  { return readTimeout; }

PINLINE void PChannel::SetWriteTimeout(const PTimeInterval & time)
  { writeTimeout = time; }

PINLINE PTimeInterval PChannel::GetWriteTimeout() const
  { return writeTimeout; }

PINLINE int PChannel::GetHandle() const
  { return os_handle; }

PINLINE PChannel::Errors PChannel::GetErrorCode(ErrorGroup group) const
  { return lastErrorCode[group]; }

PINLINE int PChannel::GetErrorNumber(ErrorGroup group) const
  { return lastErrorNumber[group]; }

PINLINE void PChannel::AbortCommandString()
  { abortCommandString = PTrue; }


///////////////////////////////////////////////////////////////////////////////
// PIndirectChannel

PINLINE PIndirectChannel::~PIndirectChannel()
  { Close(); }

PINLINE PChannel * PIndirectChannel::GetReadChannel() const
  { return readChannel; }

PINLINE PChannel * PIndirectChannel::GetWriteChannel() const
  { return writeChannel; }


///////////////////////////////////////////////////////////////////////////////
// PDirectory

PINLINE PDirectory::PDirectory()
  : PFilePathString("."), scanMask(PFileInfo::AllFiles) { Construct(); }

PINLINE PDirectory::PDirectory(const char * cpathname)  
  : PFilePathString(cpathname), scanMask(PFileInfo::AllFiles) { Construct(); }
  
PINLINE PDirectory::PDirectory(const PString & pathname)
  : PFilePathString(pathname), scanMask(PFileInfo::AllFiles) { Construct(); }
  
PINLINE PDirectory & PDirectory::operator=(const PString & str)
  { AssignContents(PDirectory(str)); return *this; }

PINLINE PDirectory & PDirectory::operator=(const char * cstr)
  { AssignContents(PDirectory(cstr)); return *this; }


PINLINE void PDirectory::DestroyContents()
  { Close(); PFilePathString::DestroyContents(); }

PINLINE PBoolean PDirectory::Exists() const
  { return Exists(*this); }

PINLINE PBoolean PDirectory::Change() const
  { return Change(*this); }

PINLINE PBoolean PDirectory::Create(int perm) const
  { return Create(*this, perm); }

PINLINE PBoolean PDirectory::Remove()
  { Close(); return Remove(*this); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PFilePath::PFilePath()
  { }

PINLINE PFilePath::PFilePath(const PFilePath & path)
  : PFilePathString(path) { }

PINLINE PFilePath & PFilePath::operator=(const PFilePath & path)
  { AssignContents(path); return *this; }

PINLINE PFilePath & PFilePath::operator=(const PString & str)
  { AssignContents(str); return *this; }

PINLINE PFilePath & PFilePath::operator=(const char * cstr)
  { AssignContents(PString(cstr)); return *this; }

PINLINE PFilePath & PFilePath::operator+=(const PString & str)
  { AssignContents(*this + str); return *this; }

PINLINE PFilePath & PFilePath::operator+=(const char * cstr)
  { AssignContents(*this + cstr); return *this; }


///////////////////////////////////////////////////////////////////////////////

PINLINE PFile::PFile()
  { os_handle = -1; removeOnClose = PFalse; }

PINLINE PFile::PFile(OpenMode mode, int opts)
  { os_handle = -1; removeOnClose = PFalse; Open(mode, opts); }

PINLINE PFile::PFile(const PFilePath & name, OpenMode mode, int opts)
  { os_handle = -1; removeOnClose = PFalse; Open(name, mode, opts); }


PINLINE PBoolean PFile::Exists() const
  { return Exists(path); }

PINLINE PBoolean PFile::Access(OpenMode mode)
  { return ConvertOSError(Access(path, mode) ? 0 : -1); }

PINLINE PBoolean PFile::Remove(PBoolean force)
  { Close(); return ConvertOSError(Remove(path, force) ? 0 : -1); }

PINLINE PBoolean PFile::Copy(const PFilePath & newname, PBoolean force)
  { return ConvertOSError(Copy(path, newname, force) ? 0 : -1); }

PINLINE PBoolean PFile::GetInfo(PFileInfo & info)
  { return ConvertOSError(GetInfo(path, info) ? 0 : -1); }

PINLINE PBoolean PFile::SetPermissions(int permissions)
  { return ConvertOSError(SetPermissions(path, permissions) ? 0 : -1); }


PINLINE const PFilePath & PFile::GetFilePath() const
  { return path; }
      

PINLINE PString PFile::GetName() const
  { return path; }

PINLINE off_t PFile::GetPosition() const
  { return _lseek(GetHandle(), 0, SEEK_CUR); }


///////////////////////////////////////////////////////////////////////////////

PINLINE PTextFile::PTextFile()
  { }

PINLINE PTextFile::PTextFile(OpenMode mode, int opts)
  { Open(mode, opts); }

PINLINE PTextFile::PTextFile(const PFilePath & name, OpenMode mode, int opts)
  { Open(name, mode, opts); }


///////////////////////////////////////////////////////////////////////////////
// PConfig

#ifdef P_CONFIG_FILE

PINLINE PConfig::PConfig(Source src)
  : defaultSection("Options") { Construct(src, "", ""); }

PINLINE PConfig::PConfig(Source src, const PString & appname)
  : defaultSection("Options") { Construct(src, appname, ""); }

PINLINE PConfig::PConfig(Source src, const PString & appname, const PString & manuf)
  : defaultSection("Options") { Construct(src, appname, manuf); }

PINLINE PConfig::PConfig(const PString & section, Source src)
  : defaultSection(section) { Construct(src, "", ""); }

PINLINE PConfig::PConfig(const PString & section, Source src, const PString & appname)
  : defaultSection(section) { Construct(src, appname, ""); }

PINLINE PConfig::PConfig(const PString & section,
                         Source src,
                         const PString & appname,
                         const PString & manuf)
  : defaultSection(section) { Construct(src, appname, manuf); }

PINLINE PConfig::PConfig(const PFilePath & filename, const PString & section)
  : defaultSection(section) { Construct(filename); }

PINLINE void PConfig::SetDefaultSection(const PString & section)
  { defaultSection = section; }

PINLINE PString PConfig::GetDefaultSection() const
  { return defaultSection; }

PINLINE PStringArray PConfig::GetKeys() const
  { return GetKeys(defaultSection); }

PINLINE PStringToString PConfig::GetAllKeyValues() const
  { return GetAllKeyValues(defaultSection); }

PINLINE void PConfig::DeleteSection()
  { DeleteSection(defaultSection); }

PINLINE void PConfig::DeleteKey(const PString & key)
  { DeleteKey(defaultSection, key); }

PINLINE PBoolean PConfig::HasKey(const PString & key) const
  { return HasKey(defaultSection, key); }

PINLINE PString PConfig::GetString(const PString & key) const
  { return GetString(defaultSection, key, PString()); }

PINLINE PString PConfig::GetString(const PString & key, const PString & dflt) const
  { return GetString(defaultSection, key, dflt); }

PINLINE void PConfig::SetString(const PString & key, const PString & value)
  { SetString(defaultSection, key, value); }

PINLINE PBoolean PConfig::GetBoolean(const PString & key, PBoolean dflt) const
  { return GetBoolean(defaultSection, key, dflt); }

PINLINE void PConfig::SetBoolean(const PString & key, PBoolean value)
  { SetBoolean(defaultSection, key, value); }

PINLINE long PConfig::GetInteger(const PString & key, long dflt) const
  { return GetInteger(defaultSection, key, dflt); }

PINLINE void PConfig::SetInteger(const PString & key, long value)
  { SetInteger(defaultSection, key, value); }

PINLINE PInt64 PConfig::GetInt64(const PString & key, PInt64 dflt) const
  { return GetInt64(defaultSection, key, dflt); }

PINLINE void PConfig::SetInt64(const PString & key, PInt64 value)
  { SetInt64(defaultSection, key, value); }

PINLINE double PConfig::GetReal(const PString & key, double dflt) const
  { return GetReal(defaultSection, key, dflt); }

PINLINE void PConfig::SetReal(const PString & key, double value)
  { SetReal(defaultSection, key, value); }

PINLINE PTime PConfig::GetTime(const PString & key) const
  { return GetTime(defaultSection, key); }

PINLINE PTime PConfig::GetTime(const PString & key, const PTime & dflt) const
  { return GetTime(defaultSection, key, dflt); }

PINLINE void PConfig::SetTime(const PString & key, const PTime & value)
  { SetTime(defaultSection, key, value); }


#endif // P_CONFIG_FILE


///////////////////////////////////////////////////////////////////////////////
// PArgList

PINLINE void PArgList::SetArgs(int argc, char ** argv)
  { SetArgs(PStringArray(argc, argv)); }

PINLINE PBoolean PArgList::Parse(const PString & theArgumentSpec, PBoolean optionsBeforeParams)
  { return Parse((const char *)theArgumentSpec, optionsBeforeParams); }

PINLINE PBoolean PArgList::HasOption(char option) const
  { return GetOptionCount(option) != 0; }

PINLINE PBoolean PArgList::HasOption(const char * option) const
  { return GetOptionCount(option) != 0; }

PINLINE PBoolean PArgList::HasOption(const PString & option) const
  { return GetOptionCount(option) != 0; }

PINLINE PINDEX PArgList::GetCount() const
  { return parameterIndex.GetSize()-shift; }

PINLINE PString PArgList::operator[](PINDEX num) const
  { return GetParameter(num); }

PINLINE PArgList & PArgList::operator<<(int sh)
  { Shift(sh); return *this; }

PINLINE PArgList & PArgList::operator>>(int sh)
  { Shift(-sh); return *this; }

///////////////////////////////////////////////////////////////////////////////
// PProcess

PINLINE PArgList & PProcess::GetArguments()
  { return arguments; }

PINLINE const PString & PProcess::GetManufacturer() const
  { return manufacturer; }

PINLINE const PString & PProcess::GetName() const
  { return productName; }

PINLINE const PFilePath & PProcess::GetFile() const
  { return executableFile; }

PINLINE int PProcess::GetMaxHandles() const
  { return maxHandles; }

PINLINE PTimerList * PProcess::GetTimerList()
  { return &timers; }

PINLINE void PProcess::SetTerminationValue(int value)
  { terminationValue = value; }

PINLINE int PProcess::GetTerminationValue() const
  { return terminationValue; }



// End Of File ///////////////////////////////////////////////////////////////
