/*
 * socket.h
 *
 * Berkley sockets ancestor class.
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

///////////////////////////////////////////////////////////////////////////////
// PSocket

  public:
    ~PSocket();
      // close a socket

    virtual PBoolean Read(void * buf, PINDEX len);
    virtual PBoolean Write(const void * buf, PINDEX len);
    virtual PBoolean Close();

  protected:
    PBoolean ConvertOSError(int status, ErrorGroup group = LastGeneralError);
    static PBoolean ConvertOSError(int error, Errors & lastError, int & osError);

  private:
#ifdef PHAS_WINSOCK
    static PBoolean WinSockStarted;
#endif


// End Of File ///////////////////////////////////////////////////////////////
