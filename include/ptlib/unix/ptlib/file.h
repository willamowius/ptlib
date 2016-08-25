/*
 * file.h
 *
 * File I/O channel class.
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

#define _read(fd,vp,st)         ::read(fd, vp, (size_t)st)
#define _write(fd,vp,st)        ::write(fd, vp, (size_t)st)
#define _fdopen                 ::fdopen
#define _lseek(fd,off,w)        ::lseek(fd, (off_t)off, w)
#define _close(fd)              ::close(fd)

///////////////////////////////////////////////////////////////////////////////
// PFile

// nothing to do

// End Of File ////////////////////////////////////////////////////////////////
