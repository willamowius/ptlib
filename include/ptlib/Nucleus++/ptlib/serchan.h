/*
 * serchan.h
 *
 * Asynchronous serial I/O channel class.
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

#ifndef _PSERIALCHANNEL

#ifndef __NUCLEUS_PLUS__
#pragma interface
#endif

#ifdef __NUCLEUS_MNT__
#include "hardware/uart.h"
#else
extern "C" {
#include "uart.h"
}
#endif

#include "../../serchan.h"
  public:
    PBoolean Close();

  private:
    DWORD  baudRate;
    BYTE   dataBits;
    Parity parityBits;
    BYTE   stopBits;
    
    UNSIGNED_CHAR smc;
    UART_INIT uart[2];
    
    virtual PBoolean Read(
      void * buf,   /// Pointer to a block of memory to receive the read bytes.
      PINDEX len    /// Maximum number of bytes to read into the buffer.
      );

    virtual int ReadChar();

    virtual PBoolean ReadAsync(
      void * buf,   /// Pointer to a block of memory to receive the read bytes.
      PINDEX len    /// Maximum number of bytes to read into the buffer.
      );

    virtual void OnReadComplete(
      void * buf, /// Pointer to a block of memory that received the read bytes.
      PINDEX len  /// Actual number of bytes to read into the buffer.
      );

    virtual PBoolean Write(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
      );

    virtual PBoolean WriteAsync(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
      );

    virtual void OnWriteComplete(
      const void * buf, /// Pointer to a block of memory to write.
      PINDEX len        /// Number of bytes to write.
      );
  };

#endif
