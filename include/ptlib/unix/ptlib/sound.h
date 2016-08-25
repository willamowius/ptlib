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

#ifdef USE_ESD
#include <ptclib/delaychan.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// declare type for sound handle dictionary

#if defined(P_MAC_MPTHREADS)
class JRingBuffer;
#endif

///////////////////////////////////////////////////////////////////////////////
// PSound

  public:
    PBoolean Close();
    PBoolean Write(const void * buf, PINDEX len);
    PBoolean Read(void * buf, PINDEX len);
  
  protected:
    PBoolean  Setup();

    static PMutex dictMutex;

    Directions direction;
    PString device;
    PBoolean isInitialised;

#if defined(P_MAC_MPTHREADS)
    JRingBuffer *mpInput;
#endif

    unsigned mNumChannels;
    unsigned mSampleRate;
    unsigned mBitsPerSample;
    unsigned actualSampleRate;

#ifdef USE_ESD
    PAdaptiveDelay writeDelay;
#endif

#ifdef P_MACOSX
    int caDevID;               // the CoreAdudio Device ID
    unsigned caNumChannels;    // number of channels the device has

    unsigned int chunkSamples; // number of samples each chunk has
    void *caCBData;            // pointer to various data for CA callbacks
                               // including caBufLen, caBuf, and so on

    int caBufLen;
    char *caBuf;
    char *consumerOffset, *producerOffset;

    void *caConverterRef;      // sample rate converter reference
    pthread_mutex_t caMutex;
    pthread_cond_t caCond;
#endif

// End Of File ////////////////////////////////////////////////////////////////
