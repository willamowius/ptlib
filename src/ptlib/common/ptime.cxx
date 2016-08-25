/*
 * ptime.cxx
 *
 * Time and date classes.
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
#include <ctype.h>

///////////////////////////////////////////////////////////////////////////////
// PTimeInterval

PTimeInterval::PTimeInterval(long millisecs,
                             long seconds,
                             long minutes,
                             long hours,
                             int days)
{
  SetInterval(millisecs, seconds, minutes, hours, days);
}


PTimeInterval::PTimeInterval(const PString & str)
{
  PStringStream strm(str);
  ReadFrom(strm);
}


PInt64 PTimeInterval::GetMilliSeconds() const
{ 
  return m_milliseconds; 
}

void PTimeInterval::SetMilliSeconds(PInt64 msecs)
{ 
  m_milliseconds = msecs;
}

PObject::Comparison PTimeInterval::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PTimeInterval), PInvalidCast);
  const PTimeInterval & other = (const PTimeInterval &)obj;

  return GetMilliSeconds() < other.GetMilliSeconds() ? LessThan :
         GetMilliSeconds() > other.GetMilliSeconds() ? GreaterThan : EqualTo;}


void PTimeInterval::PrintOn(ostream & stream) const
{
  int precision = (int)stream.precision();

  Formats fmt = NormalFormat;
  if ((stream.flags()&ios::scientific) != 0)
    fmt = SecondsOnly;
  else if (precision < 0) {
    fmt = IncludeDays;
    precision = -precision;
  }

  stream << AsString(precision, fmt, (int)stream.width());
}


void PTimeInterval::ReadFrom(istream &strm)
{
  long day = 0;
  long hour = 0;
  long min = 0;
  float sec;
  strm >> sec;
  while (strm.peek() == ':') {
    day = hour;
    hour = min;
    min = (long)sec;
    strm.get();
    strm >> sec;
  }

  SetInterval(((long)(sec*1000))%1000, (int)sec, min, hour, day);
}


PString PTimeInterval::AsString(int precision, Formats format, int width) const
{
  PStringStream str;

  str << right << setfill('0');

  if (precision > 3)
    precision = 3;
  else if (precision < 0)
    precision = 0;

  PInt64 ms = GetMilliSeconds();
  if (ms < 0) {
    str << '-';
    ms = -ms;
  }

  if (format == SecondsOnly) {
    switch (precision) {
      case 1 :
        str << ms/1000 << '.' << (int)(ms%1000+50)/100;
        break;

      case 2 :
        str << ms/1000 << '.' << setw(2) << (int)(ms%1000+5)/10;
        break;

      case 3 :
        str << ms/1000 << '.' << setw(3) << (int)(ms%1000);
        break;

      default :
        str << (ms+500)/1000;
    }

    return str;
  }

  PBoolean hadPrevious = PFalse;
  long tmp;

  if (format == IncludeDays) {
    tmp = (long)(ms/86400000);
    if (tmp > 0 || width > (precision+10)) {
      str << tmp << 'd';
      hadPrevious = PTrue;
    }

    tmp = (long)(ms%86400000)/3600000;
  }
  else
    tmp = (long)(ms/3600000);

  if (hadPrevious || tmp > 0 || width > (precision+7)) {
    if (hadPrevious)
      str.width(2);
    str << tmp << ':';
    hadPrevious = PTrue;
  }

  tmp = (long)(ms%3600000)/60000;
  if (hadPrevious || tmp > 0 || width > (precision+4)) {
    if (hadPrevious)
      str.width(2);
    str << tmp << ':';
    hadPrevious = PTrue;
  }

  if (hadPrevious)
    str.width(2);
  str << (long)(ms%60000)/1000;

  switch (precision) {
    case 1 :
      str << '.' << (int)(ms%1000)/100;
      break;

    case 2 :
      str << '.' << setw(2) << (int)(ms%1000)/10;
      break;

    case 3 :
      str << '.' << setw(3) << (int)(ms%1000);
  }

  return str;
}


DWORD PTimeInterval::GetInterval() const
{
  PInt64 msecs = GetMilliSeconds();

  if (msecs <= 0)
    return 0;

  if (msecs >= UINT_MAX)
    return UINT_MAX;

  return (DWORD)msecs;
}


void PTimeInterval::SetInterval(PInt64 millisecs,
                                long seconds,
                                long minutes,
                                long hours,
                                int days)
{
  PInt64 milliseconds;

  milliseconds = days;
  milliseconds *= 24;
  milliseconds += hours;
  milliseconds *= 60;
  milliseconds += minutes;
  milliseconds *= 60;
  milliseconds += seconds;
  milliseconds *= 1000;
  milliseconds += millisecs;

  SetMilliSeconds(milliseconds);
}


///////////////////////////////////////////////////////////////////////////////
// PTime

static time_t p_mktime(struct tm * t, int zone)
{
  // mktime returns GMT, calculated using input_time - timezone. However, this
  // assumes that the input time was a local time. If the input time wasn't a
  // local time, then we have have to add the local timezone (without daylight
  // savings) and subtract the specified zone offset to get GMT
  // and then subtract
  t->tm_isdst = PTime::IsDaylightSavings() ? 1 : 0;
  time_t theTime = mktime(t);
  if (theTime == (time_t)-1)
    theTime = 0;
  else if (zone != PTime::Local) {
    theTime += PTime::GetTimeZone()*60;  // convert to local time
    if (theTime > (time_t) zone*60)
      theTime -= zone*60;           // and then to GMT
  }
  return theTime;
}


PTime::PTime(const PString & str)
{
  PStringStream s(str);
  ReadFrom(s);
}


PTime::PTime(int second, int minute, int hour,
             int day,    int month,  int year,
             int zone)
{
  microseconds = 0;

  struct tm t;
  PAssert(second >= 0 && second <= 59, PInvalidParameter);
  t.tm_sec = second;
  PAssert(minute >= 0 && minute <= 59, PInvalidParameter);
  t.tm_min = minute;
  PAssert(hour >= 0 && hour <= 23, PInvalidParameter);
  t.tm_hour = hour;
  PAssert(day >= 1 && day <= 31, PInvalidParameter);
  t.tm_mday = day;
  PAssert(month >= 1 && month <= 12, PInvalidParameter);
  t.tm_mon = month-1;
  PAssert(year >= 1970 && year <= 2038, PInvalidParameter);
  t.tm_year   = year-1900;

  theTime = p_mktime(&t, zone);
}


PObject::Comparison PTime::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PTime), PInvalidCast);
  const PTime & other = (const PTime &)obj;
  if (theTime < other.theTime)
    return LessThan;
  if (theTime > other.theTime)
    return GreaterThan;
  if (microseconds < other.microseconds)
    return LessThan;
  if (microseconds > other.microseconds)
    return GreaterThan;
  return EqualTo;
}


void PTime::SetTimestamp(time_t seconds, long usecs)
{ 
  microseconds = usecs%1000000;
  theTime = seconds + usecs/1000000;
}


PString PTime::AsString(TimeFormat format, int zone) const
{ 
  if (format >= NumTimeStrings)
    return "Invalid format : " + AsString("yyyy-MM-dd T hh:mm:ss Z");

  switch (format) {
    case RFC1123 :
      return AsString("wwwe, dd MMME yyyy hh:mm:ss z", zone);
    case RFC3339 :
      return AsString("yyyy-MM-ddThh:mm:ssZZ", zone);
    case ShortISO8601 :
      return AsString("yyyyMMddThhmmssZ", zone);
    case LongISO8601 :
      return AsString("yyyy-MM-dd T hh:mm:ss Z", zone);
    case EpochTime :
      return psprintf("%u.%06lu", theTime, microseconds);
    default :
      break;
  }

  PString fmt, dsep;

  PString tsep = GetTimeSeparator();
  PBoolean is12hour = GetTimeAMPM();

  switch (format ) {
    case LongDateTime :
    case LongTime :
    case MediumDateTime :
    case ShortDateTime :
    case ShortTime :
      if (!is12hour)
        fmt = "h";

      fmt += "h" + tsep + "mm";

      switch (format) {
        case LongDateTime :
        case LongTime :
          fmt += tsep + "ss";

        default :
          break;
      }

      if (is12hour)
        fmt += "a";
      break;

    default :
      break;
  }

  switch (format ) {
    case LongDateTime :
    case MediumDateTime :
    case ShortDateTime :
      fmt += ' ';
      break;

    default :
      break;
  }

  switch (format ) {
    case LongDateTime :
    case LongDate :
      fmt += "wwww ";
      switch (GetDateOrder()) {
        case MonthDayYear :
          fmt += "MMMM d, yyyy";
          break;
        case DayMonthYear :
          fmt += "d MMMM yyyy";
          break;
        case YearMonthDay :
          fmt += "yyyy MMMM d";
      }
      break;

    case MediumDateTime :
    case MediumDate :
      fmt += "www ";
      switch (GetDateOrder()) {
        case MonthDayYear :
          fmt += "MMM d, yy";
          break;
        case DayMonthYear :
          fmt += "d MMM yy";
          break;
        case YearMonthDay :
          fmt += "yy MMM d";
      }
      break;

    case ShortDateTime :
    case ShortDate :
      dsep = GetDateSeparator();
      switch (GetDateOrder()) {
        case MonthDayYear :
          fmt += "MM" + dsep + "dd" + dsep + "yy";
          break;
        case DayMonthYear :
          fmt += "dd" + dsep + "MM" + dsep + "yy";
          break;
        case YearMonthDay :
          fmt += "yy" + dsep + "MM" + dsep + "dd";
      }
      break;

    default :
      break;
  }

  if (zone != Local)
    fmt += " z";

  return AsString(fmt, zone);
}


PString PTime::AsString(const char * format, int zone) const
{
  if (!IsValid())
    return PString::Empty();

  PAssert(format != NULL, PInvalidParameter);

  PBoolean is12hour = strchr(format, 'a') != NULL;

  PStringStream str;
  str.fill('0');

  // the localtime call automatically adjusts for daylight savings time
  // so take this into account when converting non-local times
  if (zone == Local)
    zone = GetTimeZone();  // includes daylight savings time
  time_t realTime = theTime + zone*60;     // to correct timezone
  struct tm ts;
  struct tm * t = os_gmtime(&realTime, &ts);

  while (*format != '\0') {
    char formatLetter = *format;
    PINDEX repeatCount = 1;
    while (*++format == formatLetter)
      repeatCount++;

    switch (formatLetter) {
      case 'a' :
        if (t->tm_hour < 12)
          str << GetTimeAM();
        else
          str << GetTimePM();
        break;

      case 'h' :
        str << setw(repeatCount) << (is12hour ? (t->tm_hour+11)%12+1 : t->tm_hour);
        break;

      case 'm' :
        str << setw(repeatCount) << t->tm_min;
        break;

      case 's' :
        str << setw(repeatCount) << t->tm_sec;
        break;

      case 'w' :
        if (repeatCount != 3 || *format != 'e')
          str << GetDayName((Weekdays)t->tm_wday, repeatCount <= 3 ? Abbreviated : FullName);
        else {
          static const char * const EnglishDayName[] = {
            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
          };
          str << EnglishDayName[t->tm_wday];
          format++;
        }
        break;

      case 'M' :
        if (repeatCount < 3)
          str << setw(repeatCount) << (t->tm_mon+1);
        else if (repeatCount > 3 || *format != 'E')
          str << GetMonthName((Months)(t->tm_mon+1),
                                    repeatCount == 3 ? Abbreviated : FullName);
        else {
          static const char * const EnglishMonthName[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
          };
          str << EnglishMonthName[t->tm_mon];
          format++;
        }
        break;

      case 'd' :
        str << setw(repeatCount) << t->tm_mday;
        break;

      case 'y' :
        if (repeatCount < 3)
          str << setw(2) << (t->tm_year%100);
        else
          str << setw(4) << (t->tm_year+1900);
        break;

      case 'z' :
      case 'Z' :
        if (repeatCount == 1 && zone == 0) {
          if (formatLetter == 'Z')
            str << 'Z';
          else
            str << "GMT";
        }
        else {
          str << (zone < 0 ? '-' : '+');
          zone = PABS(zone);
          str << setw(2) << (zone/60);
          if (repeatCount > 0)
            str << ':';
          str << setw(2) << (zone%60);
        }
        break;

      case 'u' :
        switch (repeatCount) {
          case 1 :
            str << (microseconds/100000);
            break;
          case 2 :
            str << setw(2) << (microseconds/10000);
            break;
          case 3 :
            str << setw(3) << (microseconds/1000);
            break;
          default :
            str << setw(6) << microseconds;
            break;
        }
        break;

      case '\\' :
        // Escaped character, put straight through to output string

      default :
        str << formatLetter;
    }
  }

  return str;
}

///////////////////////////////////////////////////////////
//
//  Time parser
//

extern "C" {

#ifdef _WIN32
#define STDAPICALLTYPE __stdcall
#else
#define STDAPICALLTYPE
#endif

// This funcctions implementation is in getdate_tab.c
// Which in turn is generated from getdate.y by yacc/bison
extern time_t STDAPICALLTYPE PTimeParse(void *, struct tm *, int);


int STDAPICALLTYPE PTimeGetChar(void * stream)
{
  return ((istream *)stream)->get();
}


void STDAPICALLTYPE PTimeUngetChar(void * stream, int c)
{
  ((istream *)stream)->putback((char)c);
}


int STDAPICALLTYPE PTimeGetDateOrder()
{
  return PTime::GetDateOrder();
}


int STDAPICALLTYPE PTimeIsMonthName(const char * str, int month, int abbrev)
{
  return PTime::GetMonthName((PTime::Months)month,
                             abbrev ? PTime::Abbreviated : PTime::FullName) *= str;
}


int STDAPICALLTYPE PTimeIsDayName(const char * str, int day, int abbrev)
{
  return PTime::GetDayName((PTime::Weekdays)day,
                             abbrev ? PTime::Abbreviated : PTime::FullName) *= str;
}


};


void PTime::ReadFrom(istream & strm)
{
  time_t now;
  struct tm timeBuf;
  time(&now);
  microseconds = 0;
  strm >> ws;
  theTime = PTimeParse(&strm, os_localtime(&now, &timeBuf), GetTimeZone(StandardTime));
}


bool PTime::Parse(const PString & str)
{
  PStringStream strm(str);
  ReadFrom(strm);
  return IsValid();
}


PTime PTime::operator+(const PTimeInterval & t) const
{
  time_t secs = theTime + t.GetSeconds();
  long usecs = (long)(microseconds + (t.GetMilliSeconds()%1000)*1000);
  if (usecs < 0) {
    usecs += 1000000;
    secs--;
  }
  else if (usecs >= 1000000) {
    usecs -= 1000000;
    secs++;
  }

  return PTime(secs, usecs);
}


PTime & PTime::operator+=(const PTimeInterval & t)
{
  theTime += t.GetSeconds();
  microseconds += (long)(t.GetMilliSeconds()%1000)*1000;
  if (microseconds < 0) {
    microseconds += 1000000;
    theTime--;
  }
  else if (microseconds >= 1000000) {
    microseconds -= 1000000;
    theTime++;
  }

  return *this;
}


PTimeInterval PTime::operator-(const PTime & t) const
{
  time_t secs = theTime - t.theTime;
  long usecs = microseconds - t.microseconds;
  if (usecs < 0) {
    usecs += 1000000;
    secs--;
  }
  else if (usecs >= 1000000) {
    usecs -= 1000000;
    secs++;
  }
  return PTimeInterval(usecs/1000, (long)secs);
}


PTime PTime::operator-(const PTimeInterval & t) const
{
  time_t secs = theTime - t.GetSeconds();
  long usecs = (long)(microseconds - (t.GetMilliSeconds()%1000)*1000);
  if (usecs < 0) {
    usecs += 1000000;
    secs--;
  }
  else if (usecs >= 1000000) {
    usecs -= 1000000;
    secs++;
  }
  return PTime(secs, usecs);
}


PTime & PTime::operator-=(const PTimeInterval & t)
{
  theTime -= t.GetSeconds();
  microseconds -= (long)(t.GetMilliSeconds()%1000)*1000;
  if (microseconds < 0) {
    microseconds += 1000000;
    theTime--;
  }
  else if (microseconds >= 1000000) {
    microseconds -= 1000000;
    theTime++;
  }
  return *this;
}


// End Of File ///////////////////////////////////////////////////////////////
