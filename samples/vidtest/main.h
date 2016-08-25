/*
 * main.h
 *
 * PWLib application header file for vidtest
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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

#ifndef _Vidtest_MAIN_H
#define _Vidtest_MAIN_H

#include <ptlib/pprocess.h>
#include <ptlib/videoio.h>

class VidTest : public PProcess
{
  PCLASSINFO(VidTest, PProcess)

  public:
    VidTest();
    virtual void Main();

 protected:
   PDECLARE_NOTIFIER(PThread, VidTest, GrabAndDisplay);

  PVideoInputDevice     * m_grabber;
  PVideoOutputDevice    * m_display;
  PVideoOutputDevice    * m_secondary;
  PList<PColourConverter> m_converters;
  PSyncPointAck           m_exitGrabAndDisplay;
  PSyncPointAck           m_pauseGrabAndDisplay;
  PSyncPoint              m_resumeGrabAndDisplay;
};


#endif  // _Vidtest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
