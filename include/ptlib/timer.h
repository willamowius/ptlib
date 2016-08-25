/*
 * timer.h
 *
 * Real time down counting time interval class.
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

#ifndef PTLIB_TIMER_H
#define PTLIB_TIMER_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

class PThread;

#include <ptlib/notifier.h>

/**A class represeting a simple timer.
   Unlike the PTimer class this does not support call back operations, nor is
   it startable and stoppable. It is intended for very simple real time
   operations where an elapsed time from a starting point is required. For
   example:

    <code>
     PSimpleTimer timeout(0, 10); // 10 seconds
     while (!timeout) {
       DoStuff();
     }
    </code>
  */
class PSimpleTimer : public PTimeInterval
{
  PCLASSINFO(PSimpleTimer, PTimeInterval);

  public:
  /**@name Construction */
  //@{
    /** Create a new timer object which will be expired the specified
        time interval after "now" in real time.
      */
    PSimpleTimer(
      long milliseconds = 0,  ///< Number of milliseconds for timer.
      int seconds = 0,        ///< Number of seconds for timer.
      int minutes = 0,        ///< Number of minutes for timer.
      int hours = 0,          ///< Number of hours for timer.
      int days = 0            ///< Number of days for timer.
    );
    PSimpleTimer(
      const PTimeInterval & time    ///< New time interval for timer.
    );
    PSimpleTimer(
      const PSimpleTimer & timer    ///< Timer to copy.
    );

    /** Restart the timer using the specified time value. It will be expired
        the specified time interval after "now" in real time.

       @return
       reference to the timer.
     */
    PSimpleTimer & operator=(
      DWORD milliseconds            ///< New time interval for timer.
    );
    PSimpleTimer & operator=(
      const PTimeInterval & time    ///< New time interval for timer.
    );
    PSimpleTimer & operator=(
      const PSimpleTimer & timer          ///< New time interval for timer.
    );
  //@}

  /**@name Control functions */
  //@{
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

       The timer will be expired the specified time interval after "now" in
       real time.
     */
    virtual void SetInterval(
      PInt64 milliseconds = 0,  ///< Number of milliseconds for interval.
      long seconds = 0,         ///< Number of seconds for interval.
      long minutes = 0,         ///< Number of minutes for interval.
      long hours = 0,           ///< Number of hours for interval.
      int days = 0              ///< Number of days for interval.
    );

    /**Stop the timer.
      */
    void Stop();

    /**Return the real time elapsed since instantiation.
      */
    PTimeInterval GetElapsed() const;

    /**Return the real time remaining before expiry.
      */
    PTimeInterval GetRemaining() const;

    /**Indicate timer has not expired.
      */
    bool IsRunning() const;

    /**Indicate timer has expired.
      */
    bool HasExpired() const;

    /**Indicate timer has expired.
      */
    operator bool() const;
  //@}

  protected:
    PTimeInterval m_startTick;
};


/**
   A class representing a system timer. The time interval ancestor value is
   the amount of time left in the timer.

   A timer on completion calls the virtual function <code>OnTimeout()</code>. This
   will in turn call the callback function provided by the instance. The user
   may either override the virtual function or set a callback as desired.
   
   A list of active timers is maintained by the applications <code>PProcess</code> 
   instance and the timeout functions are executed in the context of a single
   thread of execution. There are many consequences of this: only one timeout
   function can be executed at a time and thus a user should not execute a
   lot of code in the timeout call-back functions or it will dealy the timely
   execution of other timers call-back functions.

   Also timers are not very accurate in sub-second delays, even though you can
   set the timer in milliseconds, its accuracy is only to -0/+250 ms. Even
   more (potentially MUCH more) if there are delays in the user call-back
   functions.

   Another trap is you cannot destroy a timer in its own call-back. There is
   code to cause an assert if you try but it is very easy to accidentally do
   this when you delete an object that contains an onject that contains the
   timer!

   Finally static timers cause race conditions on start up and termination and
   should be avoided.
 */
class PTimer : public PTimeInterval
{
  PCLASSINFO(PTimer, PTimeInterval);

  public:
    typedef unsigned IDType;

  /**@name Construction */
  //@{
    /** Create a new timer object and start it in one shot mode for the
       specified amount of time. If the time was zero milliseconds then the
       timer is {\b not} started, ie the callback function is not executed
       immediately.
      */
    PTimer(
      long milliseconds = 0,  ///< Number of milliseconds for timer.
      int seconds = 0,        ///< Number of seconds for timer.
      int minutes = 0,        ///< Number of minutes for timer.
      int hours = 0,          ///< Number of hours for timer.
      int days = 0            ///< Number of days for timer.
    );
    PTimer(
      const PTimeInterval & time    ///< New time interval for timer.
    );
    PTimer(
      const PTimer & timer    ///< Timer to copy.
    );

    /** Restart the timer in one shot mode using the specified time value. If
       the timer was already running, the "time left" is simply reset.

       @return
       reference to the timer.
     */
    PTimer & operator=(
      DWORD milliseconds            ///< New time interval for timer.
    );
    PTimer & operator=(
      const PTimeInterval & time    ///< New time interval for timer.
    );
    PTimer & operator=(
      const PTimer & timer          ///< New time interval for timer.
    );

    /** Destroy the timer object, removing it from the applications timer list
       if it was running.
     */
    virtual ~PTimer();
  //@}

  /**@name Control functions */
  //@{
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

    /** Start a timer in continous cycle mode. Whenever the timer runs out it
       is automatically reset to the time specified. Thus, it calls the
       notification function every time interval.
     */
    void RunContinuous(
      const PTimeInterval & time    // New time interval for timer.
    );

    /** Stop a running timer. The timer will not call the notification function
       and is reset back to the original timer value. Thus when the timer
       is restarted it begins again from the beginning.

       The wait flag indicates that the function should wait for the timeout
       callback to complete before returning. That way external logic can be
       assured there is no race condition. However, under some circumstances
       this can cause a deadlock if the timeout function tries to acquire a
       mutex the calling thread already has, so an aysnchronouse version is
       provided. It is then the responsibility of the caller to handle the
       race condition with the timeout function.
     */
    void Stop(
      bool wait = true  
    );

    /** Determine if the timer is currently running. This really is only useful
       for one shot timers as repeating timers are always running.
       
       @return
       true if timer is still counting.
     */
    PBoolean IsRunning() const;

    /** Pause a running timer. This differs from the <code>Stop()</code> function in
       that the timer may be resumed at the point that it left off. That is
       time is "frozen" while the timer is paused.
     */
    void Pause();

    /** Restart a paused timer continuing at the time it was paused. The time
       left at the moment the timer was paused is the time until the next
       call to the notification function.
     */
    void Resume();

    /** Determine if the timer is currently paused.

       @return
       true if timer paused.
     */
    PBoolean IsPaused() const;

    /** Restart a timer continuing from the time it was initially.
     */
    void Reset();

    /** Get the time this timer was set to initially.
     */
    const PTimeInterval & GetResetTime() const;
  //@}

  /**@name Notification functions */
  //@{
    /**This function is called on time out. That is when the system timer
       processing decrements the timer from a positive value to less than or
       equal to zero. The interval is then reset to zero and the function
       called.

       Please note that the application should not execute large amounts of
       code in this call back or the accuracy of ALL timers can be severely
       impacted.

       The default behaviour of this function is to call the <code>PNotifier</code> 
       callback function.
     */
    virtual void OnTimeout();

    /** Get the current call back function that is called whenever the timer
       expires. This is called by the <code>OnTimeout()</code> function.

       @return
       current notifier for the timer.
     */
    const PNotifier & GetNotifier() const;

    /** Set the call back function that is called whenever the timer expires.
       This is called by the <code>OnTimeout()</code> function.
     */
    void SetNotifier(
      const PNotifier & func  // New notifier function for the timer.
    );
  //@}

  /**@name Global real time functions */
  //@{
    /** Get the number of milliseconds since some arbtrary point in time. This
       is a platform dependent function that yields a real time counter.
       
       Note that even though this function returns milliseconds, the value may
       jump in minimum quanta according the platforms timer system, eg under
       MS-DOS and MS-Windows the values jump by 55 every 55 milliseconds. The
       <code>Resolution()</code> function may be used to determine what the minimum
       time interval is.
    
       @return
       millisecond counter.
     */
    static PTimeInterval Tick();

    /** Get the smallest number of milliseconds that the timer can be set to.
       All actual timing events will be rounded up to the next value. This is
       typically the platforms internal timing units used in the <code>Tick()</code>
       function.
       
       @return
       minimum number of milliseconds per timer "tick".
     */
    static unsigned Resolution();
  //@}

  /**@name Member access */
  //@{
    /**Return number of milliseconds left in timer.
      */
    PInt64 GetMilliSeconds() const;

    /**Return absolute time timer will expire.
      */
    PInt64 GetAbsoluteTime() const { return m_absoluteTime; }
  //@}

    // Internal functions.
    IDType GetTimerId() const { return m_timerId; }
    PAtomicInteger::IntegerType GetNextSerialNumber() { return ++m_serialNumber; }

  private:
    void Construct();

    /* Start or restart the timer from the <code>resetTime</code> variable.
       This is an internal function.
     */
    void StartRunning(
      PBoolean once   // Flag for one shot or continuous.
    );

    /* Process the timer decrementing it by the delta amount and calling the
       <code>OnTimeout()</code> when zero. This is used internally by the
       <code>PTimerList::Process()</code> function.
     */
    void Process(
      PInt64 now             // time consider as "now"
    );

    // Member variables

    // Callback function for expired timers.
    PNotifier m_callback;

    // The time to reset a timer to when RunContinuous() is called.
    PTimeInterval m_resetTime;

    // Timer operates once then stops.
    PBoolean m_oneshot;

    // Timer state.
    enum { Stopped, Running, Paused } m_state;

    friend class PTimerList;              // needed for Process
    class PTimerList * m_timerList;  

    IDType m_timerId;
    PAtomicInteger m_serialNumber;
    PInt64 m_absoluteTime;

// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/timer.h"
#else
#include "unix/ptlib/timer.h"
#endif
};

#endif // PTLIB_TIMER_H


// End Of File ///////////////////////////////////////////////////////////////
