/*
 * main.h
 *
 * PWLib application header file for PluginTest
 *
 * Copyright 2003 Equivalence
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _PluginTest_MAIN_H
#define _PluginTest_MAIN_H




class PluginTest : public PProcess
{
  PCLASSINFO(PluginTest, PProcess)

  public:
    PluginTest();
    void Main();
};


#endif  // _PluginTest_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
