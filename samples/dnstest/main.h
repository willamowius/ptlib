/*
 * main.h
 *
 * PWLib application header file for DNSTest
 *
 * Copyright 2003 Equivalence
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _DNSTest_MAIN_H
#define _DNSTest_MAIN_H




class DNSTest : public PProcess
{
  PCLASSINFO(DNSTest, PProcess)

  public:
    DNSTest();
    void Main();
};


#endif  // _DNSTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
