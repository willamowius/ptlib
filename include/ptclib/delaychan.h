/*
 * delaychan.h
 *
 * Class for implementing a serial queue channel in memory.
 *
 * Portable Windows Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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

#ifndef PTLIB_DELAYCHAN_H
#define PTLIB_DELAYCHAN_H
#include <ptlib/contain.h>
#include <ptlib/object.h>
#include <ptlib/timeint.h>
#include <ptlib/ptime.h>
#include <ptlib/indchan.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif


/** Class for implementing an "adaptive" delay.
    This class will cause the the caller to, on average, delay
    the specified number of milliseconds between calls. This can
    be used to simulate hardware timing for a sofwtare only device

  */


class PAdaptiveDelay : public PObject
{ 
  PCLASSINFO(PAdaptiveDelay, PObject);
  
  public:

  /**@name Construction */
  //@{
    /**Create a new adaptive delay with the specified parameters.

       The maximum slip time can also be set later using SetMaximumSlip.
      */
    PAdaptiveDelay(
      unsigned maximumSlip = 0,   ///< Maximum slip time in milliseconds
      unsigned minimumDelay = 0   ///< Minimum delay (usually OS time slice)
    );
  //@}

  /**@name Operating Parameters */
  //@{
    /**Set the number of milliseconds that the delay may "catch up" by
       using zero delays. This is caused by the Delay() function not
       being called for a time by external factors.

       If @a maximumSlip is 0, this feature is disabled.
      */
    void SetMaximumSlip(PTimeInterval maximumSlip)
    { jitterLimit = maximumSlip; }

    /**Get the current slip time. */
    PTimeInterval GetMaximumSlip() const
    { return jitterLimit; }
  //@}

  /**@name Functionality */
  //@{
    /**Wait until the specified number of milliseconds have elapsed from
       the previous call (on average). The first time the function is called,
       no delay occurs. If the maximum slip time is set and the caller
       is "too late", the timer is restarted automatically and no delay
       occurs.

       If the calculated delay is less than the OS timer resolution
       specified on costruction, no delay occurs now ("better sooner
       than later" strategy).

       @return
       true if we are "too late" of @a time milliseconds (unrelated to
       the maximum slip time).
      */
    PBoolean Delay(int time);

    /**Invalidate the timer. The timing of this function call is not
       important, the timer will restart at the next call to Delay().
      */
    void Restart();
  //@}
 
  protected:
    PBoolean   firstTime;
    PTime  targetTime;

    PTimeInterval  jitterLimit;
    PTimeInterval  minimumDelay;
};


/** Class for implementing a "delay line" channel.
    This indirect channel can be placed in a channel I/O chain to limit the
    speed of I/O. This can be useful if blocking is not available and buffers
    could be overwritten if the I/O occurs at full speed.

    There are two modes of operation. In stream more, data can be read/written
    no faster than a fixed time for a fixed number of bytes. So, for example,
    you can say than 320 bytes must take 20 milliseconds, and thus if the
    application writes 640 byets it will delay 40 milliseconds before the next
    write.

    In frame mode, the rate limiting applies to individual read or write
    operations. So you can say that each read takes 30 milliseconds even if
    on 4 bytes is read, and the same time if 24 bytes are read.
  */
class PDelayChannel : public PIndirectChannel
{
    PCLASSINFO(PDelayChannel, PIndirectChannel);
  public:
  /**@name Construction */
  //@{
    enum Mode {
      DelayReadsOnly,
      DelayWritesOnly,
      DelayReadsAndWrites
    };

    /** Create a new delay channel with the specified delays. A value of zero
        for the numBytes parameter indicates that the delay is in frame mode.

        The maximum skip time is the number of milliseconds that the delay
        may "catch up" by using zero delays. This is caused by the Read() or
        Write() not being called for a time by external factors.
      */
    PDelayChannel(
      Mode mode,                  ///< Mode for delay channel
      unsigned frameDelay,        ///< Delay time in milliseconds
      PINDEX frameSize = 0,       ///< Bytes to apply to the delay time.
      unsigned maximumSlip = 250, ///< Maximum slip time in milliseconds
      unsigned minimumDelay = 10  ///< Minimim delay (usually OS time slice)
    );
    
    /** Create a new delay channel with the specified delays and channel. A value of zero
    for the numBytes parameter indicates that the delay is in frame mode.

    The maximum skip time is the number of milliseconds that the delay
    may "catch up" by using zero delays. This is caused by the Read() or
    Write() not being called for a time by external factors.
     */
    PDelayChannel(
        PChannel &channel,          ///< channel to use 
        Mode mode,                  ///< Mode for delay channel
        unsigned frameDelay,        ///< Delay time in milliseconds
        PINDEX frameSize = 0,       ///< Bytes to apply to the delay time.
        unsigned maximumSlip = 250, ///< Maximum slip time in milliseconds
        unsigned minimumDelay = 10  ///< Minimim delay (usually OS time slice)
                 );    
  //@}


  /**@name Overrides from class PChannel */
  //@{
    /**Low level read from the file channel. The read timeout is ignored for
       file I/O. The GetLastReadCount() function returns the actual number
       of bytes read.

       The GetErrorCode() function should be consulted after Read() returns
       false to determine what caused the failure.

       @return
       true indicates that at least one character was read from the channel.
       false means no bytes were read due to timeout or some other I/O error.
     */
    virtual PBoolean Read(
      void * buf,   ///< Pointer to a block of memory to receive the read bytes.
      PINDEX len    ///< Maximum number of bytes to read into the buffer.
    );

    /**Low level write to the file channel. The write timeout is ignored for
       file I/O. The GetLastWriteCount() function returns the actual number
       of bytes written.

       The GetErrorCode() function should be consulted after Write() returns
       false to determine what caused the failure.

       @return true if at least len bytes were written to the channel.
     */
    virtual PBoolean Write(
      const void * buf, ///< Pointer to a block of memory to write.
      PINDEX len        ///< Number of bytes to write.
    );
  //@}


  protected:
    virtual void Wait(PINDEX count, PTimeInterval & nextTick);

    Mode          mode;
    unsigned      frameDelay;
    PINDEX        frameSize;
    PTimeInterval maximumSlip;
    PTimeInterval minimumDelay;

    PTimeInterval nextReadTick;
    PTimeInterval nextWriteTick;
};


#endif // PTLIB_DELAYCHAN_H


// End Of File ///////////////////////////////////////////////////////////////
