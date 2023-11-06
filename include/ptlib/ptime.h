/*
 * ptime.h
 *
 * Time and date class.
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

#ifndef PTLIB_TIME_H
#define PTLIB_TIME_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


///////////////////////////////////////////////////////////////////////////////
// System time and date class

class PTimeInterval;


/**This class defines an absolute time and date. It has a number of time and
   date rendering and manipulation functions. It is based on the standard C
   library functions for time. Thus it is based on a number of seconds since
   1 January 1970.
 */
class PTime : public PObject
{
  PCLASSINFO(PTime, PObject);

  public:
  /**@name Construction */
  //@{
    /** Time Zone special codes. The value for a time zone is usually in minutes
        from UTC, this enum are special values for specific areas.
      */
    enum {
      /// Universal Coordinated Time.
      UTC   = 0,
      /// Greenwich Mean Time, effectively UTC.
      GMT   = UTC,
      /// Local Time.
      Local = 9999
    };

    /**Create a time object instance.
       This initialises the time with the current time in the current time zone.
     */
    PTime() { SetCurrentTime(); }

    /**Create a time object instance.
       This initialises the time to the specified time.
     */
    PTime(
      time_t tsecs,     ///< Time in seconds since 00:00:00 1/1/70 UTC
      long usecs = 0    ///< microseconds part of time.
    ) { theTime = tsecs; microseconds = usecs; }

    /**Create a time object instance.
       This initialises the time to the specified time, parsed from the
       string. The string may be in many different formats, for example:
          "5/03/1999 12:34:56"
          "15/06/1999 12:34:56"
          "15/06/01 12:34:56 PST"
          "5/06/02 12:34:56"
          "5/23/1999 12:34am"
          "5/23/00 12:34am"
          "1999/23/04 12:34:56"
          "Mar 3, 1999 12:34pm"
          "3 Jul 2004 12:34pm"
          "12:34:56 5 December 1999"
          "10 minutes ago"
          "2 weeks"
     */
    PTime(
      const PString & str   ///< Time and data as a string
    );

    /**Create a time object instance.
       This initialises the time to the specified time.
     */
    PTime(
      int second,           ///< Second from 0 to 59.
      int minute,           ///< Minute from 0 to 59.
      int hour,             ///< Hour from 0 to 23.
      int day,              ///< Day of month from 1 to 31.
      int month,            ///< Month from 1 to 12.
      int year,             ///< Year from 1970 to 2038
      int tz = Local        ///< local time or UTC
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the time on the heap. It is the responsibility of the
       caller to delete the created object.
    
       @return
       pointer to new time.
     */
    PObject * Clone() const;

    /**Determine the relative rank of the specified times. This ranks the
       times as you would expect.
       
       @return
       rank of the two times.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Other time to compare against.
    ) const;

    /**Output the time to the stream. This uses the <code>AsString()</code> function
       with the <code>ShortDateTime</code> parameter.
     */
    virtual void PrintOn(
      ostream & strm    ///< Stream to output the time to.
    ) const;

    /**Input the time from the specified stream. If a parse error occurs the
       time is set to the current time. The string may be in many different
       formats, for example:
          "5/03/1999 12:34:56"
          "15/06/1999 12:34:56"
          "15/06/01 12:34:56 PST"
          "5/06/02 12:34:56"
          "5/23/1999 12:34am"
          "5/23/00 12:34am"
          "1999/23/04 12:34:56"
          "Mar 3, 1999 12:34pm"
          "3 Jul 2004 12:34pm"
          "12:34:56 5 December 1999"
          "10 minutes ago"
          "2 weeks"
     */
    virtual void ReadFrom(
      istream & strm    ///< Stream to input the time from.
    );
  //@}

  /**@name Access functions */
  //@{
    /**Determine if the timestamp is valid.
       This will return true if the timestamp can be represented as a time
       in the epoch. The epoch is the 1st January 1970.

       In practice this means the time is > 13 hours to allow for time zones.
      */
    PBoolean IsValid() const;

    /**Get the total microseconds since the epoch. The epoch is the 1st
       January 1970.

       @return
       microseconds.
     */
    PInt64 GetTimestamp() const;

    /**Set the the objects time with the current time in the current time zone.
      */
    void SetCurrentTime();

    /**Set the time in seconds and microseconds.
      */
    void SetTimestamp(
      time_t seconds,
      long usecs = 0
    );

    /**Get the total seconds since the epoch. The epoch is the 1st
       January 1970.

       @return
       seconds.
     */
    time_t GetTimeInSeconds() const;

    /**Get the microsecond part of the time.

       @return
       integer in range 0..999999.
     */
    long GetMicrosecond() const;

    /**Get the second of the time.

       @return
       integer in range 0..59.
     */
    int GetSecond() const;

    /**Get the minute of the time.

       @return
       integer in range 0..59.
     */
    int GetMinute() const;

    /**Get the hour of the time.

       @return
       integer in range 0..23.
     */
    int GetHour() const;

    /**Get the day of the month of the date.

       @return
       integer in range 1..31.
     */
    int GetDay() const;

    /// Month codes.
    enum Months {
      January = 1,
      February,
      March,
      April,
      May,
      June,
      July,
      August,
      September,
      October,
      November,
      December
    };

    /**Get the month of the date.

       @return
       enum for month.
     */
    Months GetMonth() const;

    /**Get the year of the date.

       @return
       integer in range 1970..2038.
     */
    int GetYear() const;

    /// Days of the week.
    enum Weekdays {
      Sunday,
      Monday,
      Tuesday,
      Wednesday,
      Thursday,
      Friday,
      Saturday
    };

    /**Get the day of the week of the date.
    
       @return
       enum for week days with 0=Sun, 1=Mon, ..., 6=Sat.
     */
    Weekdays GetDayOfWeek() const;

    /**Get the day in the year of the date.
    
       @return
       integer from 1..366.
     */
    int GetDayOfYear() const;

    /**Determine if the time is in the past or in the future.

       @return
       true if time is before the current real time.
     */
    PBoolean IsPast() const;

    /**Determine if the time is in the past or in the future.

       @return
       true if time is after the current real time.
     */
    PBoolean IsFuture() const;
  //@}

  /**@name Time Zone configuration functions */
  //@{
    /**Get flag indicating daylight savings is current.
    
       @return
       true if daylight savings time is active.
     */
    static PBoolean IsDaylightSavings();

    /// Flag for time zone adjustment on daylight savings.
    enum TimeZoneType {
      StandardTime,
      DaylightSavings
    };

    /// Get the time zone offset in minutes.
    static int GetTimeZone();
    /**Get the time zone offset in minutes.
       This is the number of minutes to add to UTC (previously known as GMT) to
       get the local time. The first form automatically adjusts for daylight
       savings time, whilst the second form returns the specified time.

       @return
       Number of minutes.
     */
    static int GetTimeZone(
       TimeZoneType type  ///< Daylight saving or standard time.
    );

    /**Get the text identifier for the local time zone .

       @return
       Time zone identifier string.
     */
    static PString GetTimeZoneString(
       TimeZoneType type = StandardTime ///< Daylight saving or standard time.
    );
  //@}

  /**@name Operations */
  //@{
    /**Add the interval to the time to yield a new time.
    
       @return
       Time altered by the interval.
     */
    PTime operator+(
      const PTimeInterval & time   ///< Time interval to add to the time.
    ) const;

    /**Add the interval to the time changing the instance.
    
       @return
       reference to the current time instance.
     */
    PTime & operator+=(
      const PTimeInterval & time   ///< Time interval to add to the time.
    );

    /**Calculate the difference between two times to get a time interval.
    
       @return
       Time intervale difference between the times.
     */
    PTimeInterval operator-(
      const PTime & time   ///< Time to subtract from the time.
    ) const;

    /**Subtract the interval from the time to yield a new time.
    
       @return
       Time altered by the interval.
     */
    PTime operator-(
      const PTimeInterval & time   ///< Time interval to subtract from the time.
    ) const;

    /**Subtract the interval from the time changing the instance.

       @return
       reference to the current time instance.
     */
    PTime & operator-=(
      const PTimeInterval & time   ///< Time interval to subtract from the time.
    );
  //@}

  /**@name String conversion functions */
  //@{
    /// Standard time formats for string representations of a time and date.
    enum TimeFormat {
      /// Internet standard format. (eg. Wed, 09 Feb 2011 11:25:58 +01:00)
      RFC1123,
      /// Another Internet standard format. (eg. 2011-02-09T11:14:41ZZ)
      RFC3339,
      /// Short form ISO standard format. (eg. 20110209T111108Z)
      ShortISO8601,
      /// Long form ISO standard format. (eg. 2011-02-09 T 11:13:06 Z)
      LongISO8601,
      /// Date with weekday, full month names and time with seconds.
      LongDateTime,
      /// Date with weekday, full month names and no time.
      LongDate,
      /// Time with seconds.
      LongTime,
      /// Date with abbreviated month names and time without seconds.
      MediumDateTime,
      /// Date with abbreviated month names and no time.
      MediumDate,
      /// Date with numeric month name and time without seconds.
      ShortDateTime,
      /// Date with numeric month and no time.
      ShortDate,
      /// Time without seconds.
      ShortTime,
      /// Epoch format (e.g. 1234476388.123456)
      EpochTime,
      NumTimeStrings
    };

    /** Convert the time to a string representation. */
    PString AsString(
      TimeFormat formatCode = RFC1123,  ///< Standard format for time.
      int zone = Local                  ///< Time zone for the time.
    ) const;

    /** Convert the time to a string representation. */
    PString AsString(
      const PString & formatStr, ///< Arbitrary format string for time.
      int zone = Local           ///< Time zone for the time.
    ) const;
    /* Convert the time to a string using the format code or string as a
       formatting template. The special characters in the formatting string
       are:
       <table border=0>
       <tr><td>h         <td>hour without leading zero
       <tr><td>hh        <td>hour with leading zero
       <tr><td>m         <td>minute without leading zero
       <tr><td>mm        <td>minute with leading zero
       <tr><td>s         <td>second without leading zero
       <tr><td>ss        <td>second with leading zero
       <tr><td>u         <td>tenths of second
       <tr><td>uu        <td>hundedths of second with leading zero
       <tr><td>uuu       <td>millisecond with leading zeros
       <tr><td>uuuu      <td>microsecond with leading zeros
       <tr><td>a         <td>the am/pm string
       <tr><td>w/ww/www  <td>abbreviated day of week name
       <tr><td>wwww      <td>full day of week name
       <tr><td>d         <td>day of month without leading zero
       <tr><td>dd        <td>day of month with leading zero
       <tr><td>M         <td>month of year without leading zero
       <tr><td>MM        <td>month of year with leading zero
       <tr><td>MMM       <td>month of year as abbreviated text
       <tr><td>MMMM      <td>month of year as full text
       <tr><td>y/yy      <td>year without century
       <tr><td>yyy/yyyy  <td>year with century
       <tr><td>z         <td>the time zone description ('GMT' for UTC)
       <tr><td>Z         <td>the time zone description ('Z' for UTC)
       <tr><td>ZZ        <td>the time zone description (':' separates hour/minute)
       </table>

       All other characters are copied to the output string unchanged.
       
       Note if there is an 'a' character in the string, the hour will be in 12
       hour format, otherwise in 24 hour format.

       @return empty string if time is invalid.
     */
    PString AsString(
      const char * formatPtr,    ///< Arbitrary format C string pointer for time.
      int zone = Local           ///< Time zone for the time.
    ) const;

    /**Parse a string representation of time.
       This initialises the time to the specified time, parsed from the
       string. The string may be in many different formats, for example:
          "5/03/1999 12:34:56"
          "15/06/1999 12:34:56"
          "15/06/01 12:34:56 PST"
          "5/06/02 12:34:56"
          "5/23/1999 12:34am"
          "5/23/00 12:34am"
          "1999/23/04 12:34:56"
          "Mar 3, 1999 12:34pm"
          "3 Jul 2004 12:34pm"
          "12:34:56 5 December 1999"
          "10 minutes ago"
          "2 weeks"
     */
    bool Parse(
      const PString & str
    );
  //@}

  /**@name Internationalisation functions */
  //@{
    /**Get the internationalised time separator.
    
       @return
       string for time separator.
     */
    static PString GetTimeSeparator();

    /**Get the internationalised time format: AM/PM or 24 hour.
    
       @return
       true is 12 hour, false if 24 hour.
     */
    static PBoolean GetTimeAMPM();

    /**Get the internationalised time AM string.
    
       @return
       string for AM.
     */
    static PString GetTimeAM();

    /**Get the internationalised time PM string.
    
       @return
       string for PM.
     */
    static PString GetTimePM();

    /// Flag for returning language dependent string names.
    enum NameType {
      FullName,
      Abbreviated
    };

    /**Get the internationalised day of week day name (0=Sun etc).
    
       @return
       string for week day.
     */
    static PString GetDayName(
      Weekdays dayOfWeek,       ///< Code for day of week.
      NameType type = FullName  ///< Flag for abbreviated or full name.
    );

    /**Get the internationalised date separator.
    
       @return
       string for date separator.
     */
    static PString GetDateSeparator();

    /**Get the internationalised month name string (1=Jan etc).
    
       @return
       string for month.
     */
    static PString GetMonthName(
      Months month,             ///< Code for month in year.
      NameType type = FullName  ///< Flag for abbreviated or full name.
    );

    /// Possible orders for date components.
    enum DateOrder {
      MonthDayYear,   ///< Date is ordered month then day then year.
      DayMonthYear,   ///< Date is ordered day then month then year.
      YearMonthDay    ///< Date is ordered year then day month then day.
    };

    /**Return the internationalised date order.
    
       @return
       code for date ordering.
     */
    static DateOrder GetDateOrder();
  //@}

    static struct tm * os_localtime(const time_t * clock, struct tm * t);
    static struct tm * os_gmtime(const time_t * clock, struct tm * t);
    /*
      Threadsafe version of localtime library call.
      We could make these calls non-static if we could put the struct tm inside the
      instance. But these calls are usually made with const objects so that's not possible,
      and we would require per-thread storage otherwise. Sigh...
    */

  protected:
    // Member variables
    /// Number of seconds since 1 January 1970.
    time_t theTime;
    long   microseconds;


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/ptime.h"
#else
#include "unix/ptlib/ptime.h"
#endif
};


#endif // PTLIB_TIME_H


// End Of File ///////////////////////////////////////////////////////////////
