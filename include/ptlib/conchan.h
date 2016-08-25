/*
 * conchan.h
 *
 * Console I/O channel class.
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

#ifndef PTLIB_CONSOLECHANNEL_H
#define PTLIB_CONSOLECHANNEL_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

///////////////////////////////////////////////////////////////////////////////
// Console Channel

/**This class defines an I/O channel that communicates via a console.
 */
class PConsoleChannel : public PChannel
{
  PCLASSINFO(PConsoleChannel, PChannel);

  public:
    enum ConsoleType {
      StandardInput,
      StandardOutput,
      StandardError
    };

  /**@name Construction */
  //@{
    /// Create a new console channel object, leaving it unopen.
    PConsoleChannel();

    /// Create a new console channel object, connecting to the I/O stream.
    PConsoleChannel(
      ConsoleType type  /// Type of console for object
    );
  //@}

  /**@name Overrides from PChannel */
  //@{
    /**Set local echo mode.
       For some classes of channel, e.g. PConsoleChannel, data read by this
       channel is automatically echoed. This disables the function so things
       like password entry can work.

       Default behaviour does nothing and return true if the channel is open.
      */
    virtual bool SetLocalEcho(
      bool localEcho
    );
  //@}

  /**@name Open functions */
  //@{
    /**Open a serial channal.
       The channel is opened it on the specified port and with the specified
       attributes.
     */
    virtual PBoolean Open(
      ConsoleType type  /// Type of console for object
    );
  //@}


// Include platform dependent part of class
#ifdef _WIN32
#include "msos/ptlib/conchan.h"
#else
#include "unix/ptlib/conchan.h"
#endif

};


#endif // PTLIB_CONSOLECHANNEL_H


// End Of File ///////////////////////////////////////////////////////////////
