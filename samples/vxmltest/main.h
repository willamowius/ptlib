/*
 * main.h
 *
 * PWLib application header file for vxmltest
 *
 * Copyright 2002 Equivalence
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _Vxmltest_MAIN_H
#define _Vxmltest_MAIN_H

#include <ptlib/pprocess.h>

class PVXMLSession;

class Vxmltest : public PProcess
{
  PCLASSINFO(Vxmltest, PProcess)

  public:
    Vxmltest();
    void Main();
    PDECLARE_NOTIFIER(PThread, Vxmltest, InputThread);

  protected:
    PBoolean inputRunning;
    PVXMLSession * vxml;
};


#endif  // _Vxmltest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
