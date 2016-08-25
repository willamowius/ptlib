/*
 * sound.h
 *
 * Sound class.
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

#ifndef _PSOUND

#ifndef __NUCLEUS_MNT__
#pragma interface
#endif

#include <string>
#include <list>
#include <fstream>

///////////////////////////////////////////////////////////////////////////////
// declare type for sound handle dictionary

class PSoundHandleEntry : public PObject {
  PCLASSINFO(PSoundHandleEntry, PObject)

  public:
    PSoundHandleEntry();

    int handle;
    int direction;

    unsigned numChannels;
    unsigned sampleRate;
    unsigned bitsPerSample;
    unsigned fragmentValue;
    PBoolean isInitialised;
};

PDICTIONARY(PSoundHandleDict, PString, PSoundHandleEntry);

///////////////////////////////////////////////////////////////////////////////
// PSound

#include "../../sound.h"

    virtual PString GetName() const;
	virtual PBoolean Read(void * buf, PINDEX len);
	virtual PBoolean Write(const void * buf, PINDEX len);
	virtual PBoolean Close();

    PString m_device;
    Directions m_Direction;
    unsigned m_numChannels;
    unsigned m_sampleRate;
    unsigned m_bitsPerSample;

    unsigned long m_soundbitereadptr;

public:
};


#endif
