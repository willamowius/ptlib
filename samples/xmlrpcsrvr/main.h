/*
 * main.h
 *
 * PWLib application header file for xmlrpcsrvr
 *
 * Copyright 2002 Equivalence
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef _Xmlrpcsrvr_MAIN_H
#define _Xmlrpcsrvr_MAIN_H

#include <ptlib/pprocess.h>
#include <ptclib/httpsvc.h>
#include <ptclib/pxmlrpcs.h>

class Xmlrpcsrvr : public PHTTPServiceProcess
{
  PCLASSINFO(Xmlrpcsrvr, PHTTPServiceProcess)

  public:
    Xmlrpcsrvr();
    void Main();
    PBoolean OnStart();
    void OnStop();
    void OnConfigChanged();
    void OnControl();
    PString GetPageGraphic();
    void AddUnregisteredText(PHTML & html);
    PBoolean Initialise(const char * initMsg);

    PDECLARE_NOTIFIER(PXMLRPCServerParms, Xmlrpcsrvr, FunctionNotifier);

  protected:
    PXMLRPCServerResource * xmlrpcServer;
};


#endif  // _Xmlrpcsrvr_MAIN_H


// End of File ///////////////////////////////////////////////////////////////
