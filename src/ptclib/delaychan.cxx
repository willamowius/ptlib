/*
 * delaychan.cxx
 *
 * Class for controlling the timing of data passing through it.
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

#ifdef __GNUC__
#pragma implementation "delaychan.h"
#endif

#include <ptlib.h>
#include <ptclib/delaychan.h>

/////////////////////////////////////////////////////////

PAdaptiveDelay::PAdaptiveDelay(unsigned _maximumSlip, unsigned _minimumDelay)
  : jitterLimit(_maximumSlip), minimumDelay(_minimumDelay)
{
  firstTime = PTrue;
}

void PAdaptiveDelay::Restart()
{
  firstTime = PTrue;
}

PBoolean PAdaptiveDelay::Delay(int frameTime)
{
  if (firstTime) {
    firstTime = PFalse;
    targetTime = PTime();   // targetTime is the time we want to delay to
    return PTrue;
  }

  if (frameTime == 0)
    return true;

  // Set the new target
  targetTime += frameTime;

  // Calculate the sleep time so we delay until the target time
  PTimeInterval delay = targetTime - PTime();
  int sleep_time = (int)delay.GetMilliSeconds();

  // Catch up if we are too late and the featue is enabled
  if (jitterLimit > 0 && sleep_time < -jitterLimit.GetMilliSeconds()) {
    unsigned i = 0;
    while (sleep_time < -jitterLimit.GetMilliSeconds()) { 
      targetTime += frameTime;
      sleep_time += frameTime;
      i++;
    }
    PTRACE (4, "AdaptiveDelay\tSkipped " << i << " frames");
  }

  // Else sleep only if necessary
  if (sleep_time > minimumDelay.GetMilliSeconds())
#if defined(P_LINUX) || defined(P_MACOSX)
    usleep(sleep_time * 1000);
#else
    PThread::Sleep(sleep_time);
#endif

  return sleep_time <= -frameTime;
}

/////////////////////////////////////////////////////////

PDelayChannel::PDelayChannel(Mode m,
                             unsigned delay,
                             PINDEX size,
                             unsigned max,
                             unsigned min)
{
  mode = m;
  frameDelay = delay;
  frameSize = size;
  maximumSlip = -PTimeInterval(max);
  minimumDelay = min;
}

PDelayChannel::PDelayChannel(PChannel &channel,
                             Mode m,
                             unsigned delay,
                             PINDEX size,
                             unsigned max,
                             unsigned min) :
   mode(m), 
   frameDelay(delay),
   frameSize(size),
   minimumDelay(min)
{
  maximumSlip = -PTimeInterval(max);
  if(Open(channel) == PFalse){
    PTRACE(1,"Delay\tPDelayChannel cannot open channel");
  }
  PTRACE(5,"Delay\tdelay = " << frameDelay << ", size = " << frameSize);
}

PBoolean PDelayChannel::Read(void * buf, PINDEX count)
{
  if (!PIndirectChannel::Read(buf, count))
    return false;

  if (mode != DelayWritesOnly)
    Wait(lastReadCount, nextReadTick);

  return true;
}


PBoolean PDelayChannel::Write(const void * buf, PINDEX count)
{
  if (!PIndirectChannel::Write(buf, count))
    return false;

  if (mode != DelayReadsOnly)
    Wait(lastWriteCount, nextWriteTick);

  return true;
}


void PDelayChannel::Wait(PINDEX count, PTimeInterval & nextTick)
{
  PTimeInterval thisTick = PTimer::Tick();

  if (nextTick == 0)
    nextTick = thisTick;

  PTimeInterval delay = nextTick - thisTick;
  if (delay > maximumSlip)
    PTRACE(6, "Delay\t" << delay);
  else {
    PTRACE(6, "Delay\t" << delay << " ignored, too large");
    nextTick = thisTick;
    delay = 0;
  }

  if (frameSize > 0)
    nextTick += count*frameDelay/frameSize;
  else
    nextTick += frameDelay;

  if (delay > minimumDelay)
    PThread::Sleep(delay);
}


// End of File ///////////////////////////////////////////////////////////////
