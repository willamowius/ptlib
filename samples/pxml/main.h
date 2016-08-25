/*
 * main.h
 *
 * PWLib application header file for PxmlTest
 *
 * Copyright 2002 David Iodice.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _PxmlTest_MAIN_H
#define _PxmlTest_MAIN_H

#include <ptlib/pprocess.h>
#include <ptclib/pxml.h>


class PxmlTest : public PProcess
{
  PCLASSINFO(PxmlTest, PProcess)

  public:
    PxmlTest();
    void Main();
};

#endif  // _PxmlTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
