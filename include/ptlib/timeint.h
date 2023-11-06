/*
 * timeint.h
 *
 * Millisecond resolution time interval class (uses 64 bit integers).
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

#ifndef PTLIB_TIMEINTERVAL_H
#define PTLIB_TIMEINTERVAL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


///////////////////////////////////////////////////////////////////////////////
// Difference between two system times

/** This class defines an arbitrary time interval to millisecond accuracy. The
   interval can be both positive and negative.
   
   A long int is used to store the time interval so it is limited to LONG_MAX
   (found in the standard C header file limits.h) milliseconds. This is
   approximately 596 hours for 32 bit integers.
   
   There is a constant, <code>PMaxTimeInterval</code> which defines the
   maximum number of milliseconds that a time interval may be.
 */
class PTimeInterval : public PObject
{
  PCLASSINFO(PTimeInterval, PObject);

  public:
  /**@name Construction */
  //@{
    /** Create a new time interval object. The time interval, in milliseconds,
       is the sum of all of the parameters. For example all of the following
       are equivalent:
<pre><code>
              PTimeInterval(120000)
              PTimeInterval(60000, 60)
              PTimeInterval(60000, 0, 1)
              PTimeInterval(0, 60, 1)
              PTimeInterval(0, 0, 2)
</code></pre>
     */
    PTimeInterval(
      PInt64 millisecs = 0   ///< Number of milliseconds for interval.
    );
    PTimeInterval(
      long millisecs,       ///< Number of milliseconds for interval.
      long seconds,         ///< Number of seconds for interval.
      long minutes = 0,     ///< Number of minutes for interval.
      long hours = 0,       ///< Number of hours for interval.
      int days = 0          ///< Number of days for interval.
    );
    PTimeInterval(
      const PString & str   ///< String representation of time interval.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /** Create a new copy of the time interval. It is the responsibility of the
       called to delete the object.
       
       @return
       new time interval on heap.
     */
    PObject * Clone() const;

    /** Rank the two time intervals. This ranks the intervals as you would
       expect for two integers.
       
       @return
       <code>EqualTo</code>, <code>LessThan</code> or <code>GreaterThan</code>
       depending on their relative rank.
     */
    virtual Comparison Compare(
      const PObject & obj   ///< Time interval to compare against.
    ) const;

    /** Output the time interval to the I/O stream. This outputs the number of
       milliseconds as a signed decimal integer number.
     */
    virtual void PrintOn(
      ostream & strm    ///< I/O stream to output the time interval.
    ) const;

    /** Input the time interval from the I/O stream. This expects the input
       to be a signed decimal integer number.
     */
    virtual void ReadFrom(
      istream & strm    ///< I/O stream to input the time interval from.
    );
  //@}

  /**@name Conversion functions */
  //@{
    enum Formats {
      NormalFormat,
      IncludeDays,
      SecondsOnly
    };

    PString AsString(
      int decimals = 3,
      Formats format = NormalFormat,
      int width = 1
    ) const;
  //@}

  /**@name Access functions */
  //@{
    /** Get the number of milliseconds for the time interval.
    
       @return
       very long integer number of milliseconds.
     */
    virtual PInt64 GetMilliSeconds() const;

    /** Set the number of milliseconds for the time interval.
     */
    virtual void SetMilliSeconds(PInt64 msecs);

    /** Get the number of whole seconds for the time interval.
    
       @return
       long integer number of seconds.
     */
    long GetSeconds() const;

    /** Get the number of whole minutes for the time interval.
    
       @return
       integer number of minutes.
     */
    long GetMinutes() const;

    /** Get the number of whole hours for the time interval.
    
       @return
       integer number of hours.
     */
    int GetHours() const;

    /** Get the number of whole days for the time interval.
    
       @return
       integer number of days.
     */
    int GetDays() const;

    /** Get the number of milliseconds for the time interval.
    
       @return
       long integer number of milliseconds.
     */
    DWORD GetInterval() const;

    /** Set the value of the time interval. The time interval, in milliseconds,
       is the sum of all of the parameters. For example all of the following
       are equivalent:
<pre><code>
              SetInterval(120000)
              SetInterval(60000, 60)
              SetInterval(60000, 0, 1)
              SetInterval(0, 60, 1)
              SetInterval(0, 0, 2)
</code></pre>
     */
    virtual void SetInterval(
      PInt64 milliseconds = 0,  ///< Number of milliseconds for interval.
      long seconds = 0,         ///< Number of seconds for interval.
      long minutes = 0,         ///< Number of minutes for interval.
      long hours = 0,           ///< Number of hours for interval.
      int days = 0              ///< Number of days for interval.
    );
  //@}

  /**@name Operations */
  //@{
    /** Unary minus, get negative of time interval.
    
       @return
       difference of the time intervals.
     */
    PTimeInterval operator-() const;

    /** Add the two time intervals yielding a third time interval.
    
       @return
       sum of the time intervals.
     */
    PTimeInterval operator+(
      const PTimeInterval & interval   ///< Time interval to add.
    ) const;

    /** Add the second time interval to the first time interval.
    
       @return
       reference to first time interval.
     */
    PTimeInterval & operator+=(
      const PTimeInterval & interval   ///< Time interval to add.
    );

    /** Subtract the two time intervals yielding a third time interval.
    
       @return
       difference of the time intervals.
     */
    PTimeInterval operator-(
      const PTimeInterval & interval   ///< Time interval to subtract.
    ) const;

    /** Subtract the second time interval from the first time interval.
    
       @return
       reference to first time interval.
     */
    PTimeInterval & operator-=(
      const PTimeInterval & interval   ///< Time interval to subtract.
    );

    /** Multiply the time interval by a factor yielding a third time interval.
    
       @return
       the time intervals times the factor.
     */
    PTimeInterval operator*(
      int factor   ///< factor to multiply.
    ) const;

    /** Multiply the time interval by a factor.
    
       @return
       reference to time interval.
     */
    PTimeInterval & operator*=(
      int factor   ///< factor to multiply.
    );

    /** Divide the time interval by another interval yielding a count.
    
       @return
       the number of times the second interval occurs in the larger.
     */
    int operator/(
      const PTimeInterval & smaller   ///< factor to divide.
    ) const;

    /** Divide the time interval by a factor yielding a third time interval.
    
       @return
       the time intervals divided by the factor.
     */
    PTimeInterval operator/(
      int factor   ///< factor to divide.
    ) const;

    /** Divide the time interval by a factor.
    
       @return
       reference to time interval.
     */
    PTimeInterval & operator/=(
      int factor   ///< factor to divide.
    );
  //@}

  /**@name Comparison functions */
  //@{
    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       true if intervals are equal.
     */
    bool operator==(
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
    bool operator==(
      long msecs    ///< Time interval as integer milliseconds to compare.
    ) const;

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       true if intervals are not equal.
     */
    bool operator!=(
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
    bool operator!=(
      long msecs    ///< Time interval as integer milliseconds to compare.
    ) const;

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       true if intervals are greater than.
     */
    bool operator> (
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
    bool operator> (
      long msecs    ///< Time interval as integer milliseconds to compare.
    ) const;

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       true if intervals are greater than or equal.
     */
    bool operator>=(
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
    bool operator>=(
      long msecs    ///< Time interval as integer milliseconds to compare.
    ) const;

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       true if intervals are less than.
     */
    bool operator< (
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
    bool operator< (
      long msecs    ///< Time interval as integer milliseconds to compare.
    ) const;

    /** Compare to the two time intervals. This is provided as an override to
       the default in PObject so that comparisons can be made to integer
       literals that represent milliseconds.

       @return
       true if intervals are less than or equal.
     */
    bool operator<=(
      const PTimeInterval & interval   ///< Time interval to compare.
    ) const;
    bool operator<=(
      long msecs    ///< Time interval as integer milliseconds to compare.
    ) const;
  //@}

  private:
  // Member variables
    /// Number of milliseconds in time interval.
    PInt64 m_milliseconds;


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/timeint.h"
#else
#include "unix/ptlib/timeint.h"
#endif
};

#endif // PTLIB_TIMEINTERVAL_H


// End Of File ///////////////////////////////////////////////////////////////
